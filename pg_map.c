#include "postgres.h"
#include "funcapi.h"

#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_proc.h"
#include "access/htup_details.h"
#include "utils/syscache.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/tqual.h"
#include "utils/fmgroids.h"

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_oid_map);
PG_FUNCTION_INFO_V1(pg_procname_map);

static AnyArrayType *anyarray_map(Oid procId, AnyArrayType *array);
static Oid get_cast_proc(Oid src, Oid dst);
static Oid get_proc_arg_oid(Oid procId);
static int32 get_typmod(Oid typeId);

Datum
pg_oid_map(PG_FUNCTION_ARGS)
{
	Oid 			procId = PG_GETARG_OID(0);
	AnyArrayType   *array = PG_GETARG_ANY_ARRAY(1);
	AnyArrayType   *result;

	result = anyarray_map(procId, array);

	PG_RETURN_ARRAYTYPE_P(result);
}

Datum
pg_procname_map(PG_FUNCTION_ARGS)
{
	text		   *pro_name = PG_GETARG_TEXT_PP(0);
	AnyArrayType   *array = PG_GETARG_ANY_ARRAY(1);
	Oid 			procId;
	AnyArrayType   *result;

	procId = DatumGetObjectId(DirectFunctionCall1(to_regprocedure,
												  PointerGetDatum(pro_name)));
	result = anyarray_map(procId, array);

	PG_RETURN_ARRAYTYPE_P(result);
}

static AnyArrayType *
anyarray_map(Oid procId, AnyArrayType *array)
{
	FmgrInfo				funcinfo;
	FunctionCallInfoData	locfcinfo;
	ArrayMapState		   *amstate;
	Oid						elemtype = AARR_ELEMTYPE(array);
	Oid						argtype = get_proc_arg_oid(procId);

	if (elemtype != argtype)
	{
		AnyArrayType *newarray = anyarray_map(get_cast_proc(elemtype,
															argtype),
											  array);
		pfree(array);
		array = newarray;
	}

	fmgr_info(procId, &funcinfo);
	amstate = (ArrayMapState *) palloc0(sizeof(ArrayMapState));

	InitFunctionCallInfoData(locfcinfo, &funcinfo, 3,
							 InvalidOid, NULL, NULL);

	locfcinfo.arg[0] = PointerGetDatum(array);
	locfcinfo.arg[1] = get_typmod(elemtype);
	locfcinfo.arg[2] = BoolGetDatum(0);
	locfcinfo.argnull[0] = false;
	locfcinfo.argnull[1] = false;
	locfcinfo.argnull[2] = false;

	return DatumGetAnyArray(array_map(&locfcinfo,
									  get_func_rettype(procId),
									  amstate));
}

static Oid
get_cast_proc(Oid src, Oid dst)
{
	Relation		cast_heap;
	HeapScanDesc	heapScan;
	Oid				castproc = InvalidOid;
	HeapTuple		tuple;
	ScanKeyData		keys[2];

	ScanKeyInit(&keys[0],
				Anum_pg_cast_castsource,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(src));

	ScanKeyInit(&keys[1],
				Anum_pg_cast_casttarget,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(dst));

	cast_heap = heap_open(CastRelationId, AccessShareLock);
	heapScan = heap_beginscan(cast_heap, SnapshotSelf, 2, keys);

	tuple = heap_getnext(heapScan, ForwardScanDirection);
	if (HeapTupleIsValid(tuple))
	{
		Datum	value;
		bool	isnull = true;

		value = heap_getattr(tuple, Anum_pg_cast_castfunc,
							 RelationGetDescr(cast_heap), &isnull);
		castproc = DatumGetObjectId(value);
	}

	heap_endscan(heapScan);
	heap_close(cast_heap, AccessShareLock);

	if (castproc != InvalidOid)
		return castproc;
	else
		elog(ERROR, "cast from %d to %d not found", src, dst);
}

static Oid
get_proc_arg_oid(Oid procId)
{
	HeapTuple	htup;
	Oid			arg = InvalidOid;

	htup = SearchSysCache1(PROCOID, ObjectIdGetDatum(procId));
	if (HeapTupleIsValid(htup))
	{
		Form_pg_proc proctup = (Form_pg_proc) GETSTRUCT(htup);
		if (proctup->proargtypes.dim1 > 0)
			arg = proctup->proargtypes.values[0];

		ReleaseSysCache(htup);
	}

	if (arg == InvalidOid)
		elog(ERROR, "can't get argument type");

	return arg;
}

static int32
get_typmod(Oid typeId)
{
	HeapTuple	htup;
	int32		typmod = 0;

	htup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(typeId));
	if (HeapTupleIsValid(htup))
	{
		Form_pg_type typtup = (Form_pg_type) GETSTRUCT(htup);
		typmod = Int32GetDatum(PointerGetDatum(typtup->typtypmod));
		ReleaseSysCache(htup);
	}
	else
		elog(ERROR, "type %d not found", typeId);

	return typmod;
}

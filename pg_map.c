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

static AnyArrayType *anyarray_map(Oid procId, AnyArrayType *array, int reclimit);
static Oid get_cast_proc(Oid src, Oid dst);
static Oid get_proc_arg_oid(Oid procId);
static bool contains(text *str, text *s);

Datum
pg_oid_map(PG_FUNCTION_ARGS)
{
	Oid 			procId = PG_GETARG_OID(0);
	AnyArrayType   *array = PG_GETARG_ANY_ARRAY(1);
	AnyArrayType   *result;

	result = anyarray_map(procId, array, 1);

	PG_RETURN_ARRAYTYPE_P(result);
}

Datum
pg_procname_map(PG_FUNCTION_ARGS)
{
	MemoryContext	oldcontext = CurrentMemoryContext;
	text		   *pro_name = PG_GETARG_TEXT_PP(0);
	AnyArrayType   *array = PG_GETARG_ANY_ARRAY(1);
	Oid 			procId = InvalidOid;
	AnyArrayType   *result;
	FmgrInfo		finfo;

	if (contains(pro_name, cstring_to_text("(")) ||
		contains(pro_name, cstring_to_text(")")))
	{
		fmgr_info(F_TO_REGPROCEDURE, &finfo);
	}
	else
		fmgr_info(F_TO_REGPROC, &finfo);

	PG_TRY();
	{
		procId = DatumGetObjectId(FunctionCall1(&finfo,
												PointerGetDatum(pro_name)));
		Assert(procId != InvalidOid);
		result = anyarray_map(procId, array, 1);

		PG_RETURN_ARRAYTYPE_P(result);
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(oldcontext);

		elog(ERROR,
			 "can't find function \"%s\"",
			 text_to_cstring(pro_name));
	}
	PG_END_TRY();
}

static AnyArrayType *
anyarray_map(Oid procId, AnyArrayType *array, int reclimit)
{
	FmgrInfo				funcinfo;
	FunctionCallInfoData	locfcinfo;
	ArrayMapState		   *amstate;
	Oid						elemtype = AARR_ELEMTYPE(array);
	Oid						argtype = get_proc_arg_oid(procId);

	if (reclimit < 0)
		elog(ERROR, "unlimited recursion");

	if (elemtype != argtype)
	{
		AnyArrayType *newarray = anyarray_map(get_cast_proc(elemtype,
															argtype),
											  array,
											  reclimit - 1);
		pfree(array);
		array = newarray;
	}

	fmgr_info(procId, &funcinfo);
	amstate = (ArrayMapState *) palloc0(sizeof(ArrayMapState));

	InitFunctionCallInfoData(locfcinfo, &funcinfo, 1,
							 InvalidOid, NULL, NULL);

	locfcinfo.arg[0] = PointerGetDatum(array);
	locfcinfo.argnull[0] = false;

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

static bool
contains(text *str, text *s)
{
	Datum result;

	result = DirectFunctionCall2(textpos,
								 PointerGetDatum(str),
								 PointerGetDatum(s));

	return DatumGetInt32(result) != 0;
}
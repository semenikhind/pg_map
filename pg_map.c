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

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_oid_map);
PG_FUNCTION_INFO_V1(pg_procname_map);

AnyArrayType *anyarray_map(Oid, AnyArrayType *);
AnyArrayType *cast_array(Oid, AnyArrayType *);

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

AnyArrayType *anyarray_map(Oid procId, AnyArrayType *array)
{
	FmgrInfo				funcinfo;
	FunctionCallInfoData	locfcinfo;
	ArrayMapState		   *amstate;
	HeapTuple				htup;

/*

    oidvector				oidArray;
	Oid						arg = 100;

	htup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(AARR_ELEMTYPE(array)));
	if (HeapTupleIsValid(htup))
	{
		Form_pg_proc proctup = (Form_pg_proc) GETSTRUCT(htup);
		oidArray.values[0] = proctup->proargtypes.values[0];
		arg = oidArray.values[0];
	}
	ReleaseSysCache(htup);

//	array = cast_array(procId, array);
	if (AARR_ELEMTYPE(array) != arg)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid oid , %d, %d", AARR_ELEMTYPE(array), arg)));
*/
	fmgr_info(procId, &funcinfo);
	amstate = (ArrayMapState *) palloc0(sizeof(ArrayMapState));

	InitFunctionCallInfoData(locfcinfo, &funcinfo, 3,
							 InvalidOid, NULL, NULL);

	htup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(AARR_ELEMTYPE(array)));
	if (HeapTupleIsValid(htup))
	{
		Form_pg_type typtup = (Form_pg_type) GETSTRUCT(htup);
		locfcinfo.arg[1] = Int32GetDatum(PointerGetDatum(typtup->typtypmod));
	}
	ReleaseSysCache(htup);

	locfcinfo.arg[0] = PointerGetDatum(array);
	locfcinfo.arg[2] = BoolGetDatum(0);
	locfcinfo.argnull[0] = false;
	locfcinfo.argnull[1] = false;
	locfcinfo.argnull[2] = false;

	return DatumGetAnyArray(array_map(&locfcinfo, get_func_rettype(procId), amstate));
}

AnyArrayType *cast_array(Oid procId, AnyArrayType *array)
{
	Oid						castsrc;
	Oid						casttrg;
	Oid						castfc;
	Relation				cast_heap;
	HeapScanDesc			heapScan;
	Datum					values[Natts_pg_cast];
	bool					nulls[Natts_pg_cast];
	HeapTuple				tuple;
	AnyArrayType		   *result;
	bool					verification;
	HeapTuple				htup;
	Oid						arg = 0;

	htup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(AARR_ELEMTYPE(array)));
	if (HeapTupleIsValid(htup))
	{
		Form_pg_proc proctup = (Form_pg_proc) GETSTRUCT(htup);
		arg = ObjectIdGetDatum(ObjectIdGetDatum(proctup->proargtypes.values[0]));
	}
	ReleaseSysCache(htup);

	result = (AnyArrayType *) palloc(sizeof(AnyArrayType));
	if (AARR_ELEMTYPE(array) != arg)
	{
		verification = true;
		cast_heap = heap_open(CastRelationId, AccessShareLock);
		heapScan = heap_beginscan(cast_heap, SnapshotSelf, 0, (ScanKey) NULL);

		while(HeapTupleIsValid(tuple = heap_getnext(heapScan, ForwardScanDirection)) && verification)
		{
			heap_deform_tuple(tuple, cast_heap->rd_att, values, nulls);
			castsrc = DatumGetObjectId(values[0]);
			if (castsrc == AARR_ELEMTYPE(array))
			{
				casttrg = DatumGetObjectId(values[1]);
				castfc = DatumGetObjectId(values[2]);
				if ((casttrg == arg) || (castfc != 0))
				{
					result = anyarray_map(castfc, array);
					verification = false;
				}
			}
		}

		heap_endscan(heapScan);
		heap_close(cast_heap, AccessShareLock);
	}
	return result;
}
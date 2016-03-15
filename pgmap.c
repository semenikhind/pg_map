#include "postgres.h"
#include "funcapi.h"

#include "catalog/pg_type.h"
#include "access/htup_details.h"
#include "utils/syscache.h"
#include "utils/array.h"

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pgmap);
PG_FUNCTION_INFO_V1(intadd2);

Datum
pgmap(PG_FUNCTION_ARGS)
{
	HeapTuple  tp;
	Oid procId = PG_GETARG_OID(0);
	AnyArrayType *array = (AnyArrayType *)PG_GETARG_ANY_ARRAY(1);

	ArrayCoerceExprState *astate = (ArrayCoerceExprState *) palloc(sizeof(ArrayCoerceExprState));
	FunctionCallInfoData *locfcinfo = (FunctionCallInfoData *) palloc(sizeof(FunctionCallInfoData));

	fmgr_info(procId, locfcinfo->flinfo);

	tp = SearchSysCache1(TYPEOID, ObjectIdGetDatum(astate->resultelemtype));
	if (HeapTupleIsValid(tp))
	{
		Form_pg_type typtup = (Form_pg_type) GETSTRUCT(tp);
		locfcinfo->arg[1] = Int32GetDatum(PointerGetDatum(typtup));
	}
	ReleaseSysCache(tp);

	locfcinfo->arg[0] = PointerGetDatum(array);
//	locfcinfo.arg[1] = Int32GetDatum(typtup);
	locfcinfo->argnull[0] = false;
	locfcinfo->argnull[1] = false;
	locfcinfo->argnull[2] = false;

	return array_map(locfcinfo, astate->resultelemtype, astate->amstate);

	PG_RETURN_ARRAYTYPE_P(array);
}

Datum
intadd2(PG_FUNCTION_ARGS)
{
	int result = PG_GETARG_INT32(0);
	PG_RETURN_INT32(result + 2);
}

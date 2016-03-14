#include "postgres.h"
#include "funcapi.h"

#include "utils/array.h"

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pgmap);
PG_FUNCTION_INFO_V1(intadd2);

Datum
pgmap(PG_FUNCTION_ARGS)
{
	FmgrInfo *finfo = (FmgrInfo *) palloc(sizeof(FmgrInfo));
	Oid retType = PG_GETARG_OID(0);
	AnyArrayType *array = (AnyArrayType *)PG_GETARG_ANY_ARRAY(1);
	ArrayMapState *amstate = (ArrayMapState *) array;

	fmgr_info(retType, finfo);

	AnyArrayType *result = (DatumGetAnyArray(DirectFunctionCall3((AnyArrayType *)array_map,
											   PointerGetDatum(fcinfo),
											   ObjectIdGetDatum(amstate->inp_extra.element_type),
											   PointerGetDatum(amstate))));

	PG_RETURN_ARRAYTYPE_P(result);
}

Datum
intadd2(PG_FUNCTION_ARGS)
{
	int result = PG_GETARG_INT32(0);
	PG_RETURN_INT32(result + 2);
}

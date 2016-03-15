#include "postgres.h"
#include "funcapi.h"

#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "access/htup_details.h"
#include "utils/syscache.h"
#include "utils/array.h"

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_map);

Datum
pg_map(PG_FUNCTION_ARGS)
{
	HeapTuple		  		htup;
	Oid 					procId = PG_GETARG_OID(0);
	AnyArrayType		   *array = (AnyArrayType *)PG_GETARG_ANY_ARRAY(1);
	ArrayMapState  		   *amstate;
	FunctionCallInfoData	locfcinfo;
	FmgrInfo				funcinfo;

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

	return array_map(&locfcinfo, get_func_rettype(procId), amstate);

	PG_RETURN_ARRAYTYPE_P(array);
}
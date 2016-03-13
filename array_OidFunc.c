#include "postgres.h"
#include "funcapi.h"

#include <ctype.h>
#ifdef _MSC_VER
#include <float.h>	
#endif
#include <math.h>

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "utils/array.h"
#include "utils/arrayaccess.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/typcache.h"

#include <string.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(array_OidFunctionCall1);

Datum array_OidFunctionCall1(PG_FUNCTION_ARGS)

{
	FmgrInfo *finfo;
	Oid retType = PG_GETARG_OID(1);
	DirectFunctionCall2(fmgr_info,
						ObjectIdGetDatum(retType),
						PointerGetDatum(finfo));
	ArrayMapState *amstate = PG_GETARG_ARRAYTYPE_P(2);

	AnyArrayType *v;
	ArrayType  *result;
	Datum	   *values;
	bool	   *nulls;
	int		   *dim;
	int			ndim;
	int			nitems;
	int			i;
	int32		nbytes = 0;
	int32		dataoffset;
	bool		hasnulls;
	Oid			inpType;
	int			inp_typlen;
	bool		inp_typbyval;
	char		inp_typalign;
	int			typlen;
	bool		typbyval;
	char		typalign;
	array_iter	iter;
	ArrayMetaState *inp_extra;
	ArrayMetaState *ret_extra;

	/* Get input array */
	if (fcinfo->nargs < 1)
		elog(ERROR, "invalid nargs: %d", fcinfo->nargs);
	if (PG_ARGISNULL(0))
		elog(ERROR, "null input array");
	v = PG_GETARG_ANY_ARRAY(0);

	inpType = AARR_ELEMTYPE(v);
	ndim = AARR_NDIM(v);
	dim = AARR_DIMS(v);
	nitems = ArrayGetNItems(ndim, dim);

	/* Check for empty array */
	if (nitems <= 0)
	{
		/* Return empty array */
		PG_RETURN_ARRAYTYPE_P(construct_empty_array(retType));
	}

	/*
	 * We arrange to look up info about input and return element types only
	 * once per series of calls, assuming the element type doesn't change
	 * underneath us.
	 */
	inp_extra = &amstate->inp_extra;
	ret_extra = &amstate->ret_extra;

	if (inp_extra->element_type != inpType)
	{
		get_typlenbyvalalign(inpType,
							 &inp_extra->typlen,
							 &inp_extra->typbyval,
							 &inp_extra->typalign);
		inp_extra->element_type = inpType;
	}
	inp_typlen = inp_extra->typlen;
	inp_typbyval = inp_extra->typbyval;
	inp_typalign = inp_extra->typalign;

	if (ret_extra->element_type != retType)
	{
		get_typlenbyvalalign(retType,
							 &ret_extra->typlen,
							 &ret_extra->typbyval,
							 &ret_extra->typalign);
		ret_extra->element_type = retType;
	}
	typlen = ret_extra->typlen;
	typbyval = ret_extra->typbyval;
	typalign = ret_extra->typalign;

	/* Allocate temporary arrays for new values */
	values = (Datum *) palloc(nitems * sizeof(Datum));
	nulls = (bool *) palloc(nitems * sizeof(bool));

	/* Loop over source data */
	array_iter_setup(&iter, v);
	hasnulls = false;

	for (i = 0; i < nitems; i++)
	{
		bool		callit = true;

		/* Get source element, checking for NULL */
		fcinfo->arg[0] = array_iter_next(&iter, &fcinfo->argnull[0], i,
									 inp_typlen, inp_typbyval, inp_typalign);

		/*
		 * Apply the given function to source elt and extra args.
		 */
		if (fcinfo->flinfo->fn_strict)
		{
			int			j;

			for (j = 0; j < fcinfo->nargs; j++)
			{
				if (fcinfo->argnull[j])
				{
					callit = false;
					break;
				}
			}
		}

		if (callit)
		{
			fcinfo->isnull = false;
			values[i] = FunctionCallInvoke(fcinfo);
		}
		else
			fcinfo->isnull = true;

		nulls[i] = fcinfo->isnull;
		if (fcinfo->isnull)
			hasnulls = true;
		else
		{
			/* Ensure data is not toasted */
			if (typlen == -1)
				values[i] = PointerGetDatum(PG_DETOAST_DATUM(values[i]));
			/* Update total result size */
			nbytes = att_addlength_datum(nbytes, typlen, values[i]);
			nbytes = att_align_nominal(nbytes, typalign);
			/* check for overflow of total request */
			if (!AllocSizeIsValid(nbytes))
				ereport(ERROR,
						(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						 errmsg("array size exceeds the maximum allowed (%d)",
								(int) MaxAllocSize)));
		}
	}

	/* Allocate and initialize the result array */
	if (hasnulls)
	{
		dataoffset = ARR_OVERHEAD_WITHNULLS(ndim, nitems);
		nbytes += dataoffset;
	}
	else
	{
		dataoffset = 0;			/* marker for no null bitmap */
		nbytes += ARR_OVERHEAD_NONULLS(ndim);
	}
	result = (ArrayType *) palloc0(nbytes);
	SET_VARSIZE(result, nbytes);
	result->ndim = ndim;
	result->dataoffset = dataoffset;
	result->elemtype = retType;
	memcpy(ARR_DIMS(result), AARR_DIMS(v), ndim * sizeof(int));
	memcpy(ARR_LBOUND(result), AARR_LBOUND(v), ndim * sizeof(int));

	/*
	 * Note: do not risk trying to pfree the results of the called function
	 */
	CopyArrayEls(result,
				 values, nulls, nitems,
				 typlen, typbyval, typalign,
				 false);

	pfree(values);
	pfree(nulls);

	PG_RETURN_ARRAYTYPE_P(result);
}
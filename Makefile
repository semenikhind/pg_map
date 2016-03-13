# contrib/array_OidFunc/Makefile

MODULE_big	= array_OidFunc
OBJS		= array_OidFunc.o $(WIN32RES)

EXTENSION = array_OidFunc
DATA = array_OidFunc--1.0.sql
PGFILEDESC = "array_OidFunc - array_OidFunctionCall1(Oid functionId, AnyArrayType array) returns AnyArrayType of functionId for all items in array"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/array_OidFunc
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# contrib/abc/Makefile

MODULE_big	= pg_map
OBJS		= pg_map.o $(WIN32RES)

EXTENSION = pg_map
DATA = pg_map--1.0.sql
PGFILEDESC = "pg_map - Python-like map"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_map
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

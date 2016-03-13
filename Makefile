# contrib/abc/Makefile

MODULE_big	= pgmap
OBJS		= pgmap.o $(WIN32RES)

EXTENSION = pgmap
DATA = pgmap--1.0.sql
PGFILEDESC = "pgmap - PostgreSQL map"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pgmap
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

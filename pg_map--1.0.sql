/* contrib/pgmap/pg_map--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_map" to load this file. \quit

CREATE FUNCTION pg_map(oid, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'pg_oid_map'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION pg_map(cstring, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'pg_procname_map'
LANGUAGE C IMMUTABLE STRICT;
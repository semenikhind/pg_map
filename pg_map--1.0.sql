/* contrib/pgmap/pg_map--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_map" to load this file. \quit

CREATE FUNCTION pg_map(oid, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'pg_map'
LANGUAGE C IMMUTABLE STRICT;
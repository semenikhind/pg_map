/* contrib/pgmap/pgmap--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgmap" to load this file. \quit

CREATE FUNCTION pgmap(oid, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'pgmap'
LANGUAGE C IMMUTABLE STRICT;

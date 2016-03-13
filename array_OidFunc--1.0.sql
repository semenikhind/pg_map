/* contrib/array_OidFunc/array_OidFunc--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION array_OidFunc" to load this file. \quit

CREATE FUNCTION array_OidFunctionCall1(oid, anyarray)
RETURNS aniarray
AS 'MODULE_PATHNAME', 'array_OidFunc'
LANGUAGE C IMMUTABLE STRICT;
/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'local_dataset.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/


#include <stdlib.h>

#include <unordered_map>

#include <ert/util/util.h>
#include <ert/util/hash.h>

#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/local_ministep.hpp>
#include <ert/enkf/row_scaling.hpp>

#define LOCAL_DATASET_TYPE_ID  6615409



struct local_dataset_struct {
  UTIL_TYPE_ID_DECLARATION;
  char      * name;
  hash_type * active_size;      /* A hash table indexed by node keys - each element is an active_list instance. */
  std::unordered_map<std::string, row_scaling> scaling;
};



UTIL_SAFE_CAST_FUNCTION(local_dataset , LOCAL_DATASET_TYPE_ID)
UTIL_IS_INSTANCE_FUNCTION(local_dataset , LOCAL_DATASET_TYPE_ID)


local_dataset_type * local_dataset_alloc( const char * name ) {
  local_dataset_type * dataset = new local_dataset_type();

  UTIL_TYPE_ID_INIT( dataset , LOCAL_DATASET_TYPE_ID );
  dataset->active_size = hash_alloc();
  dataset->name  = util_alloc_string_copy( name );

  return dataset;
}

void local_dataset_free( local_dataset_type * dataset ) {
  free(dataset->name);
  hash_free( dataset->active_size );
  delete dataset;
}

local_dataset_type * local_dataset_alloc_copy( local_dataset_type * src_dataset , const char * copy_name ) {
  local_dataset_type * copy_dataset = local_dataset_alloc( copy_name );

  hash_iter_type * size_iter = hash_iter_alloc( src_dataset->active_size );
  while (!hash_iter_is_complete( size_iter )) {
    const char * key = hash_iter_get_next_key( size_iter );
    active_list_type * active_list = active_list_alloc_copy( (const active_list_type *) hash_get( src_dataset->active_size , key ) );
    hash_insert_hash_owned_ref( copy_dataset->active_size , key , active_list , active_list_free__);
  }
  hash_iter_free( size_iter );

  for (const auto& scaling_pair : src_dataset->scaling)
    copy_dataset->scaling.insert( scaling_pair );

  return copy_dataset;
}



void local_dataset_free__( void * arg ) {
  local_dataset_type * local_dataset = local_dataset_safe_cast( arg );
  local_dataset_free( local_dataset );
}

const char * local_dataset_get_name( const local_dataset_type * dataset) {
  return dataset->name;
}



void local_dataset_add_node(local_dataset_type * dataset, const char *node_key) {
  if (hash_has_key( dataset->active_size , node_key ))
    util_abort("%s: tried to add existing node key:%s \n",__func__ , node_key);

  hash_insert_hash_owned_ref( dataset->active_size , node_key , active_list_alloc( ) , active_list_free__);
}

bool local_dataset_has_key(const local_dataset_type * dataset, const char * key) {
  return hash_has_key( dataset->active_size , key );
}


void local_dataset_del_node( local_dataset_type * dataset , const char * node_key) {
  hash_del( dataset->active_size , node_key );
}


void local_dataset_clear( local_dataset_type * dataset) {
  hash_clear( dataset->active_size );
}

const row_scaling * local_dataset_get_row_scaling(const local_dataset_type * dataset, const char * key) {
  auto scaling_iter = dataset->scaling.find( key );
  if (scaling_iter != dataset->scaling.end())
    return &scaling_iter->second;

  return nullptr;
}


bool local_dataset_has_row_scaling(const local_dataset_type * dataset, const char * key) {
  return (dataset->scaling.count(key) != 0);
}


row_scaling_type * local_dataset_get_or_create_row_scaling(local_dataset_type * dataset, const char * key) {
  auto scaling_iter = dataset->scaling.find( key );
  if (scaling_iter == dataset->scaling.end()) {
    if (!hash_has_key(dataset->active_size, key))
      throw std::invalid_argument("Tried to create row_scaling object for unknown key");

    dataset->scaling.emplace( key, row_scaling{});
  }
  return &dataset->scaling[key];
}


active_list_type * local_dataset_get_node_active_list(const local_dataset_type * dataset , const char * node_key ) {
  active_list_type * al = (active_list_type *) hash_get( dataset->active_size , node_key );  /* Fails hard if you do not have the key ... */
  return al;
}

stringlist_type * local_dataset_alloc_keys( const local_dataset_type * dataset ) {
  return hash_alloc_stringlist( dataset->active_size );
}

void local_dataset_summary_fprintf( const local_dataset_type * dataset , FILE * stream) {
{
  hash_iter_type * data_iter = hash_iter_alloc( dataset->active_size );
  while (!hash_iter_is_complete( data_iter )) {
    const char * data_key          = hash_iter_get_next_key( data_iter );
    fprintf(stream , "NAME OF DATA:%s,", data_key );

    active_list_type * active_list = (active_list_type *)hash_get( dataset->active_size , data_key );
    active_list_summary_fprintf( active_list , local_dataset_get_name(dataset) , data_key , stream);
  }
  hash_iter_free( data_iter );
 }
}


int local_dataset_get_size( const local_dataset_type * dataset ) {
  return hash_get_size( dataset->active_size );
}


hash_iter_type * local_dataset_alloc_iter(const local_dataset_type * dataset) {
  return hash_iter_alloc(dataset->active_size);
}


std::vector<std::string> local_dataset_unscaled_keys(const local_dataset_type * dataset) {
  std::vector<std::string> keys;
  hash_iter_type *node_iter = hash_iter_alloc( dataset->active_size );

  while (!hash_iter_is_complete( node_iter )) {
    const char * key = hash_iter_get_next_key( node_iter );
    if (!local_dataset_has_row_scaling(dataset, key))
      keys.emplace_back( key );
  }

  hash_iter_free( node_iter );
  return keys;
}


std::vector<std::string> local_dataset_scaled_keys(const local_dataset_type * dataset) {
  std::vector<std::string> keys;
  for (const auto& [key, _] : dataset->scaling) {
    (void)_;
    keys.push_back(key);
  }
  return keys;
}
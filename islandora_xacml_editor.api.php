<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Define custom queries to be used when batching through "children" of objects.
 *
 * @param AbstractObject $object
 *   A AbstractObject.
 *
 * @return array
 *   An array of queries to be returned to the islandora_xacml_editor module.
 *   Each entry in the array is an associate array itself with the following
 *   structure:
 *   - type: The type of query to be executed either itql or sparql.
 *   - query: The defined query string. The field we expect returned is that of
 *     "object". I.e select ?object.
 *   - description: Human-readable description used in populating the options
 *   dropdown.
 */
function hook_islandora_xacml_editor_child_query($object) {
  return array(
    'sample_query' => array(
      'type' => 'itql',
      'query' => 'select $object from <#ri> where $object <fedora-rels-ext:isMemberOfCollection> <info:fedora/islandora:root>',
      'description' => t('Sweet query bro.'),
    ),
  );
}

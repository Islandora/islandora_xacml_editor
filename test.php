<?php

include_once('XacmlParser.inc');
include_once('XacmlWriter.inc');

$xacml = array(
  'PolicyId' => 'Islandora-Editor-Policy-v1.0',
  'RuleCombiningAlgId' => 'urn:oasis:names:tc:xacml:1.0:rule-combining-algorithm:first-applicable',
  'rules' => array(
    array(
      'id' => 'denyapi-access-to-datastream-except-to-user-or-role',
      'effect' => 'Deny',
      'methods' => array('getDatastreamDissemination'),
      'dsids' => array('AboutStacks.pdf'),
      'users' => array('usera', 'userb'),
      'roles' => array('rolea', 'roleb'),
      'mimes' => array('image/tiff', 'audio/x-wave'),
    ),
    array(
      'id' => 'denyapi-except-to-user-or-role',
      'effect' => 'Deny',
      'methods' => array('ingest', 'modifyDatastreamByReference', 'modifyDatastreamByValue', 'modifyDisseminator', 'purgeObject', 'purgeDatastream', 'purgeDisseminator', 'setDatastreamState', 'setDisseminatorState', 'setDatastreamVersionable', 'addDatastream', 'addDisseminator'),
      'dsids' => array(),
      'mimes' => array(),
      'users' => array('userb', 'userc'),
      'roles' => array('roleb', 'rolec'),
    ),
  ),
);

print XacmlWriter::toXML($xacml,TRUE);

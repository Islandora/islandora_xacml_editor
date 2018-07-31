<?php

/**
 * @file
 * Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_xacml_editor_drush_command() {
  $items = array();

  $items['islandora_xacml_editor_apply_policy'] = array(
    'aliases' => array('ixeap'),
    'description' => dt('Apply XACML policy to target object.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'examples' => array(
      'Apply policy.xml to \'islandora:57\', use traversal to target child objects.' => 'drush -v --user=1 islandora_xacml_editor_apply_policy --policy=/tmp/policy.xml --pid=islandora:57 --traversal',
    ),
    'options' => array(
      'policy' => array(
        'description' => dt('The path to an XML file containing the XACML policy configuration to be applied.'),
        'required' => TRUE,
      ),
      'pid' => array(
        'description' => dt('The PID of the object to apply the policy configuration to.'),
        'required' => TRUE,
      ),
      'traversal' => array(
        'description' => dt('Optional. Whether to apply policies to the target object\'s children (shallow traversal is not supported for collection objects). Disabled by default.'),
        'required' => FALSE,
      ),
    ),
  );

  $items['islandora_xacml_editor_force_policy_inheritance'] = array(
    'aliases' => array('ixefpi'),
    'description' => dt('Force all child objects to inherit target object\'s XACML policy configuration.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'examples' => array(
      'Enforce policy inheritance to all immediate children of \'islandora:root\' object.' => 'drush -v --user=1 islandora_xacml_editor_force_policy_inheritance --pid=islandora:root --shallow_traversal',
    ),
    'options' => array(
      'pid' => array(
        'description' => dt('The PID of the parent object. Must have a \'POLICY\' datastream.'),
        'required' => TRUE,
      ),
      'shallow_traversal' => array(
        'description' => dt('Optional. If the target object is a collection, use shallow traversal to target only the immediate children. Disabled by default.'),
        'required' => FALSE,
      ),
    ),
  );

  return $items;
}

/**
 * Command callback to apply XACML policy.
 */
function drush_islandora_xacml_editor_apply_policy() {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $object = islandora_object_load(drush_get_option('pid'));

  if (!$object) {
    drush_log(dt('Error loading object.'), 'error');
    return;
  }

  $policy = drush_get_option('policy');

  if (file_exists($policy) && is_file($policy)) {
    $xml = file_get_contents($policy);

    if (!$xml) {
      drush_log(dt('Could not read policy file at @policy', array('@policy' => $policy)), 'error');
      return;
    }
  }
  else {
    drush_log(dt('File path provided does not exist or is not a file.'), 'error');
    return;
  }

  $traversal = drush_get_option('traversal', FALSE);

  if ($traversal) {
    $query_array = array();

    foreach (islandora_build_hook_list('islandora_xacml_editor_child_query', $object->models) as $hook) {
      $temp = module_invoke_all($hook, $object);

      if (!empty($temp)) {
        $query_array = array_merge_recursive($query_array, $temp);
      }
    }

    if (empty($query_array)) {
      drush_log('No child queries found, skipping traversal...', 'notice');
    }
  }

  $batch = array(
    'operations' => array(
      array(
        'islandora_xacml_editor_batch_function',
        array($xml, $object->id, reset($query_array)),
      ),
    ),
    'finished' => 'islandora_xacml_editor_batch_finished',
    'file' => drupal_get_path('module', 'islandora_xacml_editor') . '/includes/batch.inc',
  );

  drush_log(dt('Please wait if many objects are being updated as this could take a few minutes.'), 'ok');

  batch_set($batch);
  drush_backend_batch_process();
}

/**
 * Command callback to enforce XACML inheritance.
 */
function drush_islandora_xacml_editor_force_policy_inheritance() {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $object = islandora_object_load(drush_get_option('pid'));

  if (!$object) {
    drush_log(dt('Error loading object.'), 'error');
    return;
  }

  if ($object['POLICY']) {
    $xml = $object['POLICY']->content;

    if (!$xml) {
      drush_log(dt('An error occurred while trying to load the \'POLICY\' for @pid.', array('@pid' => $object->id)), 'error');
      return;
    }
  }
  else {
    drush_log(dt('No \'POLICY\' datastream found for @pid.', array('@pid' => $object->id)), 'error');
    return;
  }

  $query_array = array();

  foreach (islandora_build_hook_list('islandora_xacml_editor_child_query', $object->models) as $hook) {
    $temp = module_invoke_all($hook, $object);

    if (!empty($temp)) {
      $query_array = array_merge_recursive($query_array, $temp);

      // If shallow traversal is enabled and the content models child query
      // found targets all children, add the shallow restriction to the query.
      if (isset($query_array['all_children']) && drush_get_option('shallow_traversal', FALSE)) {
        $query_array['all_children']['restricted_cmodels'] = array('islandora:collectionCModel');
      }
    }
  }

  if (empty($query_array)) {
    drush_log('No child query found from object\'s content model.', 'error');
    return;
  }

  $batch = array(
    'operations' => array(
      array(
        'islandora_xacml_editor_batch_function',
        array($xml, $object->id, reset($query_array)),
      ),
    ),
    'finished' => 'islandora_xacml_editor_batch_finished',
    'file' => drupal_get_path('module', 'islandora_xacml_editor') . '/includes/batch.inc',
  );

  drush_log(dt('Please wait if many objects are being updated as this could take a few minutes.'), 'ok');

  batch_set($batch);
  drush_backend_batch_process();
}

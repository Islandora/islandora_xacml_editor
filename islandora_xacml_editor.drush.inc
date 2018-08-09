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
      'Apply policy.xml to \'islandora:57\' and use traversal to target child objects.' => 'drush -v --user=1 islandora_xacml_editor_apply_policy --policy=/tmp/policy.xml --pid=islandora:57 --traversal',
    ),
    'options' => array(
      'policy' => array(
        'description' => dt('The path to an XML file containing the XACML policy configuration to be applied. It is expected that this policy file be generated from the UI\'s XACML Editor.'),
        'required' => TRUE,
      ),
      'pid' => array(
        'description' => dt('The PID of the object to apply the policy configuration to.'),
        'required' => TRUE,
      ),
      'traversal' => array(
        'description' => dt('Optional. When enabled, the policy configuration will be applied to the target object\'s children (shallow traversal is not supported for collection objects). Disabled by default.'),
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
    return drush_set_error('Invalid object', dt('An error occurred while trying to load \'@pid\'.', array('@pid' => drush_get_option('pid'))));
  }

  $policy = drush_get_option('policy');

  if (file_exists($policy) && is_file($policy)) {
    $xml = file_get_contents($policy);

    if (!$xml) {
      return drush_set_error('Failed to read file', dt('Could not read policy file from @policy', array('@policy' => $policy)));
    }
  }
  else {
    return drush_set_error('Invalid policy parameter', dt('File path provided does not exist or is not a file.'));
  }

  $traversal = drush_get_option('traversal', FALSE);

  if ($traversal) {
    $query_array = array();

    foreach (islandora_build_hook_list('islandora_xacml_editor_child_query', $object->models) as $hook) {
      $temp = module_invoke_all($hook, $object);

      if (!empty($temp)) {
        $query_array = array_merge_recursive($query_array, $temp);
        // Need to reset the array as we expect only one query.
        $query_array = reset($query_array);
      }
    }

    if (empty($query_array)) {
      drush_log('No child queries found, skipping traversal...', 'notice');
    }

    islandora_xacml_editor_process_batch($xml, $object->id, $query_array);
  }
  else {
    module_load_include('inc', 'islandora_xacml_editor', 'includes/batch');

    $policy_update = new IslandoraUpdatePolicy($object->id, $xml);
    $success = $policy_update->updatePolicy();

    if (!$success) {
      return drush_set_error('Failed to load policy', dt('An error occurred while trying to update the \'POLICY\' datastream for @pid.', array('@pid' => $object->id)));
    }

    drush_print(dt('The \'POLICY\' configuration has been applied to @pid!', array('@pid' => $object->id)));
  }
}

/**
 * Command callback to enforce XACML inheritance.
 */
function drush_islandora_xacml_editor_force_policy_inheritance() {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $object = islandora_object_load(drush_get_option('pid'));

  if (!$object) {
    return drush_set_error('Error loading object', dt('An error occurred while trying to load \'@pid\'.', array('@pid' => drush_get_option('pid'))));
  }

  if ($object['POLICY']) {
    $xml = $object['POLICY']->content;

    if (!$xml) {
      return drush_set_error('Failed to load policy', dt('An error occurred while trying to load the \'POLICY\' datastream for @pid.', array('@pid' => $object->id)));
    }
  }
  else {
    return drush_set_error('No policy datastream', dt('No \'POLICY\' datastream found for @pid.', array('@pid' => $object->id)));
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
    return drush_set_error('No child query found', dt('No child query found for object\'s content model.'));
  }

  islandora_xacml_editor_process_batch($xml, $object->id, reset($query_array));
}

/**
 * Build and process batch operation.
 *
 * @param string $xml
 *   String containing the XACML policy configuration markup.
 * @param string $pid
 *   The PID of the target object.
 * @param array $query_array
 *   Array containing query child query, type and description.
 */
function islandora_xacml_editor_process_batch($xml, $pid, array $query_array) {
  $batch = array(
    'operations' => array(
      array(
        'islandora_xacml_editor_batch_function',
        array($xml, $pid, $query_array),
      ),
    ),
    'finished' => 'islandora_xacml_editor_batch_finished',
    'file' => drupal_get_path('module', 'islandora_xacml_editor') . '/includes/batch.inc',
  );

  drush_print(dt('Please wait if many objects are being updated as this could take a few minutes.'));

  batch_set($batch);
  drush_backend_batch_process();
}

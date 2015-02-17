<?php

/**
 * @file
 * Drush hook implementations and associated functions.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_xacml_api_drush_command() {
  $command = array();

  $command['islandora-xacml-api-relationships-update-batch'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
    'options' => array(
      'limit' => array(
        'value' => 'optional',
        'description' => dt('The max number of objects for which to query at a time. (affects memory usage, defaults to 100)'),
      ),
    ),
    'drupal dependenies' => array(
      'islandora',
      'islandora_xacml_api',
    ),
    'examples' => array(
      'drush -u 1 islandora-xacml-api-relationships-update-batch',
    ),
  );

  return $command;
}

/**
 * Command callback; start the update batch.
 */
function drush_islandora_xacml_api_relationships_update_batch() {
  $batch = array(
    'operations' => array(
      array(
        'islandora_xacml_api_relationships_update_batch_operation',
        array(
          drush_get_option('limit', 100),
        ),
      ),
    ),
  );

  batch_set($batch);

  drush_backend_batch_process();
}

/**
 * Batch operation; update a PID.
 */
function islandora_xacml_api_relationships_update_batch_operation($limit, &$context) {
  if (!variable_get('islandora_xacml_api_save_relationships', TRUE)) {
    drush_log(t('Relationships are disabled, nothing to do.'));
  }
  $sandbox =& $context['sandbox'];

  // XXX: Can occasionally get be initialized as a different user...
  drupal_static_reset('islandora_get_tuque_connection');
  $tuque = islandora_get_tuque_connection();

  $query = <<<EOQ
SELECT ?object ?created
FROM <#ri>
WHERE {
  ?object <fedora-view:disseminates> ?ds ;
          <fedora-model:createdDate> ?created .
  ?ds <fedora-view:disseminationType> <info:fedora/*/POLICY> .
  !filter
}
ORDER BY ?created

EOQ;

  if (!isset($sandbox['total'])) {
    $sandbox['progress'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery(format_string($query, array(
      '!filter' => '',
    )), 'sparql');
    if ($sandbox['total'] == 0) {
      // Nothing to process.
      $context['finished'] = 1;

      $context['message'] = t('Nothing to fix.');
      return;
    }
    $sandbox['result_stash'] = array();
  }

  if (empty($sandbox['result_stash'])) {
    $limited_query = format_string($query, array(
      '!filter' => (isset($sandbox['last_date']) ?
        "FILTER(?created > \"{$sandbox['last_date']}\"^^<xsd:dateTime>)" :
        ''),
    )) . <<<EOQ
LIMIT $limit
EOQ;

    drush_log(dt('Querying for @count objects.', array('@count' => $limit)));
    $sandbox['result_stash'] = $tuque->repository->ri->sparqlQuery($limited_query);
    if (empty($sandbox['result_stash'])) {
      // Ran out of items early?
      $context['finished'] = 1;

      $context['message'] = t('Updated @count PID(s) and ran out of items early... Somebody manually updated a document?', array(
        '@count' => $sandbox['total'],
      ));
      return;
    }
  }

  $result = array_shift($sandbox['result_stash']);
  $updating_pid = $result['object']['value'];
  $sandbox['last_date'] = $result['created']['value'];
  $object_to_update = islandora_object_load($updating_pid);

  module_load_include('inc', 'islandora_xacml_api', 'includes/islandora_xacml');
  $xacml = new IslandoraXacml($object_to_update, $object_to_update['POLICY']->content);
  $xacml->writeBackToFedora();

  watchdog('islandora_xacml_api', 'Rewrote policy and relationships for @pid.', array(
    '@pid' => $updating_pid,
  ));

  $sandbox['progress'] = min($sandbox['total'], $sandbox['progress'] + 1);
  $context['finished'] = $sandbox['progress'] / $sandbox['total'];

  $context['message'] = t('Updated @progress of @total.', array(
    '@progress' => $sandbox['progress'],
    '@total' => $sandbox['total'],
  ));
}

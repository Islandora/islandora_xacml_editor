<?php

/**
 * @file
 * Rules integration.
 */

/**
 * Implements hook_rules_action_info().
 */
function islandora_xacml_api_rules_action_info() {
  $action = array();

  $action['islandora_xacml_api_rules_load'] = array(
    'label' => t('Load the XACML "POLICY" stream from the given object.'),
    'group' => t('Islandora XACML API'),
    'parameter' => array(
      'object' => array(
        'type' => 'islandora_object',
        'label' => t('The object for/from which to load the policy.'),
      ),
    ),
    'provides' => array(
      'xacml' => array(
        'type' => 'islandora_xacml',
        'label' => t('The loaded XACML policy.'),
      ),
    ),
  );
  $action['islandora_xacml_api_rules_save'] = array(
    'label' => t('Save a loaded policy back to its object.'),
    'group' => t('Islandora XACML API'),
    'parameter' => array(
      'xacml' => array(
        'type' => 'islandora_xacml',
        'label' => t('The XACML policy to save.'),
      ),
    ),
  );

  return $action;
}

/**
 * Rules action callback; load a XACML policy from an object.
 */
function islandora_xacml_api_rules_load(AbstractObject $object) {
  return array(
    'xacml' => new IslandoraXacml($object),
  );
}

/**
 * Rules action callback; save a XACML policy back to Fedora.
 */
function islandora_xacml_api_rules_save(IslandoraXacml $xacml) {
  $xacml->writeBackToFedora();
}

/**
 * Getter helper for our islandora_xacml type.
 */
function islandora_xacml_api_rules_info_getter($data, array $options, $name, $type, $info) {
  if ($data instanceof IslandoraXacml) {
    $map = array(
      'manage' => $data->managementRule,
      'view' => $data->viewingRule,
      'datastream' => $data->datastreamRule,
    );
    return $map[$name];
  }
  elseif ($data instanceof XacmlRule) {
    $rule = $data->getRuleArray();
    return $rule[$name];
  }
}

/**
 * Setter helper for our islandora_xacml type.
 */
function islandora_xacml_api_rules_info_setter(&$data, $name, $value, $langcode, $type, $info) {
  $function_map = array(
    'users' => 'addUser',
    'roles' => 'addRole',
    'mimes' => 'addMimetype',
    'mimeregexs' => 'addMimetypeRegex',
    'dsids' => 'addDsid',
    'dsidregexs' => 'addDsidRegex',
  );

  $function = $function_map[$name];
  $data->clear($name);
  $data->$function($value);
}

/**
 * No-op function for use as a setter.
 *
 * The top level "struct" objects require something, even if it does nothing...
 */
function islandora_xacml_api_rules_info_noop_setter() {
}

/**
 * Implements hook_rules_data_info().
 */
function islandora_xacml_api_rules_data_info() {
  $data = array();

  $data['islandora_xacml'] = array(
    'label' => t('Islandora XACML object'),
    'wrap' => TRUE,
    'property info' => array(
      'manage' => array(
        'label' => t('Management Rule'),
        'type' => 'struct',
        'computed' => TRUE,
        'getter callback' => 'islandora_xacml_api_rules_info_getter',
        'setter callback' => 'islandora_xacml_api_rules_info_noop_setter',
        'property info' => array(
          'users' => array(
            'label' => t('List of users in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'roles' => array(
            'label' => t('List of roles in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
        ),
      ),
      'view' => array(
        'label' => t('Viewing Rule'),
        'type' => 'struct',
        'computed' => TRUE,
        'getter callback' => 'islandora_xacml_api_rules_info_getter',
        'setter callback' => 'islandora_xacml_api_rules_info_noop_setter',
        'property info' => array(
          'users' => array(
            'label' => t('List of users in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'roles' => array(
            'label' => t('List of roles in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
        ),
      ),
      'datastream' => array(
        'label' => t('Datastream Viewing Rule'),
        'type' => 'struct',
        'computed' => TRUE,
        'getter callback' => 'islandora_xacml_api_rules_info_getter',
        'setter callback' => 'islandora_xacml_api_rules_info_noop_setter',
        'property info' => array(
          'users' => array(
            'label' => t('List of users in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'roles' => array(
            'label' => t('List of roles in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'mimes' => array(
            'label' => t('List of mimes in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'mimeregexs' => array(
            'label' => t('List of mimeregexs in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'dsids' => array(
            'label' => t('List of dsids in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
          'dsidregexs' => array(
            'label' => t('List of dsidregexs in this rule'),
            'type' => 'list<text>',
            'computed' => TRUE,
            'getter callback' => 'islandora_xacml_api_rules_info_getter',
            'setter callback' => 'islandora_xacml_api_rules_info_setter',
          ),
        ),
      ),
    ),
  );

  return $data;
}

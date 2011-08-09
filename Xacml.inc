<?php

class Xacml {

  protected $xacml;

  /**
   * The constructor for the XACML object. This will create a new blank
   * XACML object that rules can be added to. There are other constructors
   * below for instantianting an object from existing XML and Rules.
   */
  function __construct() {
    $rules = array();
  }

  /**
   * Instantiate a new object from a XML string.
   *
   * @param $xml the XACML XML string.
   * @throws XacmlException
   * @return new Xacml object.
   */
  static function fromXML($xml) {
    module_load_include('inc', 'islandora_xacml_editor', 'XacmlParser');
    $xacml_object = new Xacml();
    $this->xacml = XacmlParser::parse($xml);
    return $xacml_object;
  }

  /**
   * Instantiate a new object using existing datastructures.
   *
   * @param $xacml
   */
  static function fromRules() {

  }

  function addRule($id, $args = array(), $effect = 'Deny') {
    $rule = array();

    // @todo sanitise the id. I'm not sure what the restrictions are.
    // @todo check that $effect is correct

    $rule['id'] = $id;
    $rule['effect'] = $effect;

    array_key_exists('users', $args) ? $rule['users'] = $args['users'] : $rule['users'] = array();
    array_key_exists('roles', $args) ? $rule['roles'] = $args['roles'] : $rule['roles'] = array();
    array_key_exists('mime', $args) ? $rule['mime'] = $args['mime'] : $rule['mime'] = array();
    array_key_exists('dsid', $args) ? $rule['dsid'] = $args['dsid'] : $rule['dsid'] = array();
    array_key_exists('methods', $args) ? $rule['methods'] = $args['methods'] : $rule['methods'] = array();

    // make sure we never lock out the admin role
    if($effect == 'Deny' && !in_array('administrator',$rule['roles'])) {
      $rule['roles'][] = 'administrator';
    }

    $dom->rules[] = $rule;
    return True;
  }

}
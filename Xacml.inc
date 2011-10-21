<?php

define('MANAGEMENT_RULE', 'deny-management-functions');
define('DATASTREAM_RULE', 'deny-dsid-mime');
define('VIEWING_RULE', 'deny-access-functions');
define('PERMIT_RULE', 'allow-everything-else');

abstract class XacmlRule {
  protected $rule;
  protected $xacml;

  protected function initializeRule($id, $effect) {
    $rule = array();

    $rule['ruleid'] = $id;
    $rule['effect'] = $effect;

    $rule['dsids'] = array();
    $rule['mimes'] = array();
    $rule['methods'] = array();
    $rule['users'] = array();
    $rule['roles'] = array();

    return $rule;
  }

  protected function setValue($type, $data) {
    if(is_array($data)) {
      $this->rule[$type] = array_merge($this->rule[$type], array_values($data));
    }
    else {
      $this->rule[$type][] = $data;
    }
  }

  protected function getValues($type) {
    return array_unique($this->rule[$type]);
  }

  protected function removeValues($type, $data) {
    if(!is_array($data)) {
      $data = array($data);
    }

    $this->rule[$type] = array_diff($this->rule[$type],$data);
  }
  
  protected function mergeParentRules($parentObject) {
    if($parentObject->isPopulated()) {
      $this->rule['users'] = array_merge($this->rule['users'], $parentObject->getUsers());
      $this->rule['roles'] = array_merge($this->rule['roles'], $parentObject->getRoles());
    }
  }

  function __construct($arg1, $xacml) {
    if(is_array($arg1)) {
      $this->rule = $arg1;
      /* remove them now, add them later */
      $this->removeValues('users', 'fedoraAdmin');
      $this->removeValues('roles', 'administrator');
    }

    $this->xacml = $xacml;
  }

  function isPopulated() {
    return $this->getUsers() || $this->getRoles();
  }

  function addUser($user) {
    $this->setValue('users', $user);
  }

  function addRole($role) {
    $this->setValue('roles', $role);
  }

  function removeUser($user) {
    $this->removeValues('users', $user);
  }

  function removeRole($role) {
    $this->removeValues('users', $user);
  }

  function getUsers() {
    return $this->getValues('users');
  }

  function getRoles() {
    return $this->getValues('roles');
  }

  function getRuleArray() {
    /* make sure fedora admin can see everything */
    $this->rule['users'][] = 'fedoraAdmin';
    $this->rule['roles'][] = 'administrator';
    $this->rule['users'] = $this->getValues('users');
    $this->rule['roles'] = $this->getValues('roles');
    return $this->rule;
  }
}

class XacmlManagementRule extends XacmlRule {
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(MANAGEMENT_RULE, 'Deny');
      $this->rule['methods'] = array(
        'addDatastream',
        'addDisseminator',
        'adminPing',
        'getDisseminatorHistory',
        'getNextPid',
        'ingest',
        'modifyDatastreamByReference',
        'modifyDatastreamByValue',
        'modifyDisseminator',
        'modifyObject',
        'purgeObject',
        'purgeDatastream',
        'purgeDisseminator',
        'setDatastreamState',
        'setDisseminatorState',
        'setDatastreamVersionable',
        'compareDatastreamChecksum',
        'serverShutdown',
        'serverStatus',
        'upload',
        'dsstate',
        'resolveDatastream',
        'reloadPolicies',
      );
    }
  }
}

class XacmlViewingRule extends XacmlRule {
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(MANAGEMENT_RULE, 'Deny');
      $this->rule['methods'] = array(
        'api-a',
        'getDatastreamHistory',
        'listObjectInResourceIndexResults',
      );
    }
  }

  function getRuleArray() {
    return $this->rule;
  }
}

class XacmlPermitEverythingRule extends XacmlRule {
  function  __construct($xacml) {
    parent::__construct(NULL, $xacml);
    $this->rule = $this->initializeRule(PERMIT_RULE, 'Permit');
  }

  function  getRuleArray() {
    return $this->rule;
  }
}

class XacmlDatastreamRule extends XacmlRule {
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(DATASTREAM_RULE, 'Deny');
      $this->rule['methods'][] = 'getDatastreamDissemination';
    }
  }

  function getRuleArray() {
    $this->mergeParentRules($this->xacml->managementRule);
    parent::getRuleArray();
    $this->rule['dsids'] = $this->getValues('dsids');
    $this->rule['mimes'] = $this->getValues('mimes');
    return $this->rule;
  }

  function addDsid($dsid) {
    $this->setValue('dsids', $dsid);
  }

  function addMimetype($mime) {
    $this->setValue('mimes', $mime);
  }

  function removeMimetype($mime) {
    $this->removeValues('mimes', $mime);
  }

  function removeDsid($dsid) {
    $this->removeValues('dsids', $dsid);
  }

  function getMimetypes() {
    return $this->getValues('mimes');
  }

  function getDsids() {
    return $this->getValues('dsids');
  }

  function isPopulated() {
    return parent::isPopulated() && ($this->getMimetype() || $this->getDsids());
  }
}

class Xacml {

  protected $xacml;
  protected $permitEverythingRule;
  public $managementRule;
  public $datastreamRule;
  public $viewingRule;

  protected function initializeXacml() {
    // create the rule array
    $xacml = array(
      'RuleCombiningAlgId' => 'urn:oasis:names:tc:xacml:1.0:rule-combining-algorithm:first-applicable',
      'rules'              => array(),
    );
    return $xacml;
  }

  /**
   * The constructor for the XACML object. Initialize new XACML object.
   *
   * @param (optional) $xacml The XACML XML as a string. If this isn't passed
   *   the constructor will instead create a new XACML object that permits
   *   everything.
   * @throws XacmlException if the XML cannot be parsed
   */
  function __construct($xacml = NULL) {
    include_once('XacmlParser.inc');

    $management_rule = NULL;
    $datastream_rule = NULL;
    $viewing_rule = NULL;

    if($xacml != NULL) {
      $this->xacml = XacmlParser::parse($xacml);

      // decide what is enabled
      foreach($this->xacml['rules'] as $rule) {
        if($rule['ruleid'] == MANAGEMENT_RULE)
          $management_rule = $rule;
        elseif ($rule['ruleid'] == DATASTREAM_RULE)
          $datastream_rule = $rule;
        elseif ($rule['ruleid'] == VIEWING_RULE)
          $viewing_rule = $rule;
      }
    }
    else {
      $this->xacml = $this->initializeXacml();
    }

    $this->datastreamRule = new XacmlDatastreamRule($datastream_rule, $this);
    $this->managementRule = new XacmlManagementRule($management_rule, $this);
    $this->viewingRule = new XacmlManagementRule($viewing_rule,$this);
    $this->permitEverythingRule = new XacmlPermitEverythingRule($this);
  }
  
  private function updateRulesArray() {
    $this->xacml['rules'] = array();

    if($this->datastreamRule->isPopulated())
      $this->xacml['rules'][] = $this->datastreamRule->getRuleArray();
    
    if($this->managementRule->isPopulated())
      $this->xacml['rules'][] = $this->managementRule->getRuleArray();
    
    if($this->viewingRule->isPopulated())
      $this->xacml['rules'][] = $this->viewingRule->getRuleArray();

    $this->xacml['rules'][] = $this->permitEverythingRule->getRuleArray();
  }

  function getDomDocument() {
    include_once('XacmlWriter.inc');
    $this->updateRulesArray();
    return XacmlWriter::toDOM($this->xacml);
  }

  function getXmlString($prettyPrint=TRUE) {
    include_once('XacmlWriter.inc');
    $this->updateRulesArray();
    return XacmlWriter::toXML($this->xacml, $prettyPrint);
  }
}
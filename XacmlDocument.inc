<?php

/**
 * @file
 * This file has functions for creating XACML v1 XML documents
 *
 */

/**
 * Class inhereted from DOMDocument, that forms the XACML document.
 * This is the base class for building any XACML document.
 */
class XacmlDocument extends DOMDocument {

  protected $root;
  protected $rules = array();

  public function __construct(
    $policyID = 'islandora_xacml_editor',
    $ruleCombiningAlgId='urn:oasis:names:tc:xacml:1.0:rule-combining-algorithm:first-applicable',
    $description = 'Policy created by the Islandora XACML Editor'
  ) {
    // call parent DomDocument constructor
    parent::__construct("1.0", "UTF-8");
    $this->formatOutput = True;

    $this->root = $this->createRootElement($policyID, $ruleCombiningAlgId);
    $this->createBaseDocument($description);
  }

  /**
   * Creates the root tag for the base XACML document.

   * @param $policyID
   *   The id for the policy being created.
   * @param $ruleCombiningAlgId
   *   The algorithm this XACML document should use for policy combining.
   *
   * @return The root element.
   */
  private function createRootElement($policyID, $ruleCombiningAlgId) {
    $root = $this->createElementNS(self::xacml, 'Policy');

    $root->setAttribute('islandora_xacml', '1.0');
    $root->setAttribute('PolicyId',$policyID);
    $root->setAttribute('RuleCombiningAlgId',$ruleCombiningAlgId);
    $root->setAttributeNS(self::xmlns, 'xmlns', self::xacml);
    $root->setAttributeNS(self::xmlns, 'xmlns:xsi', self::xsi);

    $this->appendChild($root);
    return $root;
  }

  /**
   * Creates the empty skeleton document that every XACML document should have. This
   * is the section that says this rule applies to everything. We do all the narrowing
   * of the scope for rules within the rules themselves.
   *
   * @code
   *  <Description>$description</Description>
   *  <Target>
   *    <Subjects>
   *      <AnySubject/>
   *    </Subjects>
   *    <Resources>
   *      <AnyResource/>
   *    </Resources>
   *    <Actions>
   *      <AnyAction/>
   *    </Actions>
   *  </Target>
   * @endcode
   *
   * @param $description
   *   The description of the XACML rules being created. This is a human readable string.
   */
  private function createBaseDocument($description) {
    $description = $this->createElementNS(self::xacml, 'Description', $description);

    $target = $this->createElementNS(self::xacml,'Target');

    $subject = $this->createElementNS(self::xacml, 'Subjects');
    $subject->appendChild($this->createElementNS(self::xacml,'AnySubject'));
    $target->appendChild($subject);

    $resource = $this->createElementNS(self::xacml, 'Resources');
    $resource->appendChild($this->createElementNS(self::xacml,'AnyResource'));
    $target->appendChild($resource);

    $action = $this->createElementNS(self::xacml, 'Actions');
    $action->appendChild($this->createElementNS(self::xacml,'AnyAction'));
    $target->appendChild($action);

    $this->root->appendChild($target);
  }

  private function createRuleRoot($rule) {
    $ruleElement = $this->createElementNS(self::xacml,"Rule");
    $ruleElement->setAttribute('RuleId',$rule['id']);
    $ruleElement->setAttribute('Effect',$rule['effect']);
    return $ruleElement;
  }

  private function createRuleTarget($rule) {
    $target = $this->createElementNS(self::xacml,"Target");
    $target->appendChild($this->createRuleTargetSubjects($rule));
    $target->appendChild($this->createRuleTargetResources($rule));
    $target->appendChild($this->createRuleTargetActions($rule));
    return $target;
  }

  private function createRuleTargetSubjects($rule) {
    $subjects = $this->createElementNS(self::xacml, "Subjects");
    $subjects->appendChild($this->createElementNS(self::xacml,"AnySubject"));
    return $subjects;
  }

  private function createRuleTargetResources($rule) {
    $resources = $this->createElementNS(self::xacml, "Resources");

    if(empty($rule['mime']) && empty($rule['dsid'])){
      $resources->appendChild($this->createElementNS(self::xacml,"AnyResource"));
    }
    else {
      foreach($rule['mime'] as $mime) {
        $resources->appendChild($this->createRuleTargetResource($mime, 'mime'));
      }
      foreach($rule['dsid'] as $dsid) {
        $resources->appendChild($this->createRuleTargetResource($dsid, 'dsid'));
      }
    }

    return $resources;
  }

  private function createRuleTargetResource($name, $type) {
    $resource = $this->createElementNS(self::xacml, 'Resource');
    $resourceMatch = $this->createElementNS(self::xacml, 'ResourceMatch');
    $resourceMatch->setAttribute('MatchId','urn:oasis:names:tc:xacml:1.0:function:string-equal');
    $resource->appendChild($resourceMatch);

    $AttributeValue = $this->createElementNS(self::xacml, 'AttributeValue', $name);
    $AttributeValue->setAttribute('DataType',"http://www.w3.org/2001/XMLSchema#string");

    $ResourceAttributeDesignator = $this->createElementNS(self::xacml, 'ResourceAttributeDesignator');
    $ResourceAttributeDesignator->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");

    switch ($type){
      case 'mime':
        $ResourceAttributeDesignator->setAttribute("AttributeId","urn:fedora:names:fedora:2.1:resource:datastream:mimeType");
        break;
      case 'dsid':
        $ResourceAttributeDesignator->setAttribute("AttributeId","urn:fedora:names:fedora:2.1:resource:datastream:id");
        break;
      default:
        return False;
    }

    $resourceMatch->appendChild($AttributeValue);
    $resourceMatch->appendChild($ResourceAttributeDesignator);

    return $resource;
  }

  private function createRuleTargetActions($rule) {
    $actions = $this->createElementNS(self::xacml, "Actions");
    if(!empty($rule['methods'])) {
      foreach($rule['methods'] as $method) {
        $actions->appendChild($this->createRuleTargetAction($method));
      }
    }
    else {
      $actions->appendChild($this->createElementNS(self::xacml,"AnyAction"));
    }

    return $actions;
  }

  private function createRuleTargetAction($method) {
    $action = $this->createElementNS(self::xacml,'Action');
    $actionMatch = $this->createElementNS(self::xacml, 'ActionMatch');
    $action->appendChild($actionMatch);
    $attributeValue = $this->createElementNS(self::xacml, "AttributeValue", "urn:fedora:names:fedora:2.1:action:id-$method");
    $attributeValue->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
    $actionAttributeDesignator = $this->createElementNS(self::xacml, "ActionAttributeDesignator");
    $actionAttributeDesignator->setAttribute("AttributeId", "urn:fedora:names:fedora:2.1:action:id");
    $actionAttributeDesignator->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
    $actionMatch->appendChild($attributeValue);
    $actionMatch->appendChild($actionAttributeDesignator);

    return $action;
  }

  private function createRuleCondition($rule) {
    $condition = $this->createElementNS(self::xacml,"Condition");
    $condition->setAttribute("FunctionId", "urn:oasis:names:tc:xacml:1.0:function:not");

    if(!empty($rule['users'])) {
      $users = $this->createRuleConditionApply($rule['users'],'user');
    }
    if(!empty($rule['roles'])) {
      $roles = $this->createRuleConditionApply($rule['roles'],'role');
    }

    if(isset($users) && isset($roles)) {
      $apply = $this->createElementNS(self::xacml, "Apply");
      $apply->setAttribute("FunctionId","urn:oasis:names:tc:xacml:1.0:function:or");
      $condition->appendChild($apply);
      $apply->appendChild($users);
      $apply->appendChild($roles);
    }
    elseif (isset($users)) {
      $condition->appendChild($users);
    }
    elseif (isset($roles)) {
      $condition->appendChild($roles);
    }
    else {
      return NULL;
    }

    return $condition;
  }

  private function createRuleConditionApply($attributes, $type) {
    $apply = $this->createElementNS(self::xacml,'Apply');
    $apply->setAttribute("FunctionId","urn:oasis:names:tc:xacml:1.0:function:string-at-least-one-member-of");

    $subjectAttribureDesignator = $this->createElementNS(self::xacml, 'SubjectAttributeDesignator');
    $subjectAttribureDesignator->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
    $subjectAttribureDesignator->setAttribute("MustBePresent","false");

    switch($type) {
      case 'role':
        $subjectAttribureDesignator->setAttribute('AttributeId',"fedoraRole");
        break;
      case 'user':
        $subjectAttribureDesignator->setAttribute('AttributeId',"urn:fedora:names:fedora:2.1:subject:loginId");
        break;
      default:
        return NULL;
    }

    $apply->appendChild($subjectAttribureDesignator);

    $stringBag = $this->createElementNS(self::xacml,"Apply");
    $stringBag->setAttribute("FunctionId","urn:oasis:names:tc:xacml:1.0:function:string-bag");
    $apply->appendChild($stringBag);

    foreach($attributes as $attribute) {
      $attrib = $this->createElementNS(self::xacml, "AttributeValue", $attribute);
      $attrib->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
      $stringBag->appendChild($attrib);
    }

    return $apply;
  }
  
  private function createRule($rule) {
    $root = $this->createRuleRoot($rule);
    $root->appendChild($this->createRuleTarget($rule));
      
    $condition = $this->createRuleCondition($rule);  
    if($condition) {
      $root->appendChild($condition);
    }
    
    return $root;
    
  }

  private function finalizeDocument() {
    foreach($this->rules as $rule) {
      $rule_xml = $this->createRule($rule);
      $this->root->appendChild($rule_xml);
    }
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

    $this->rules[] = $rule;
    return True;
  }

  /**
   * Override the saveXML parent function to make sure rules are added before printing.
   */
  function saveXML(){
    $this->finalizeDocument();
    return parent::saveXML();
  }
}

$document = new XacmlDocument();


$document->addRule('denyapi-access-to-datastream-except-to-user-or-role',
  array(
    'mime' => array('image/tiff', 'audio/x-wav'),
    'dsid' => array('Isbull_1919.pdf'),
    'methods' => array('getDatastreamDissemination'),
    'users' => array('fedoraAdmin1','owner_id1'),
    'roles' => array('manager_role1', 'approver_role1'),
  )
);

$document->addRule('denyapi-access-to-datastream-except-to-user-or-role',
  array(
    'mime' => array('image/tiff', 'audio/x-wav'),
    'dsid' => array('Isbull_1919.pdf'),
    'methods' => array('getDatastreamDissemination'),
    'users' => array('fedoraAdmin2','owner_id2'),
    'roles' => array('manager_role2', 'approver_role2'),
  )
);

$document->addRule('permit-everything-else', array(), 'Permit');

include_once("XacmlParser.inc");

$zap = $document->saveXML();
print $zap;
print_r(XacmlParser::parse($zap));

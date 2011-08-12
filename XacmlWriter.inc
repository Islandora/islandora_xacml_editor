<?php

/**
 * @file
 * This file has functions for writing the XACML class datastructures out to XML.
 *
 */

include_once('XacmlConstants.inc');

/**
 * Class for writing XACML editor datasctructures out to XML.
 */
class XacmlWriter {

  /**
   * Function takes a $xacml datastructure as input and returns
   * a DOMDocument containing the XACML xml.
   *
   * @param $xacml
   *   $xacml datastructure
   *
   * @return
   *   DomDocument
   */
  public static function toDOM($xacml) {
    $dom = new DomDocument("1.0", "UTF-8");

    XacmlWriter::createRootElement($dom, $xacml);
    XacmlWriter::createTarget($dom, $xacml);
    XacmlWriter::createRules($dom, $xacml);

    return $dom;
  }
  
  /**
   * Function takes a $xacml datastructure as input and returns
   * an string containing the XACML xml.
   *
   * @param $xacml
   *   $xacml datastructure
   * @param $prettyprint
   *   Boolean. Determines if the output XML is formatted. Default: FALSE.
   *
   * @return
   *   String containing the XACML as XML.
   */
  public static function toXML($xacml, $prettyprint=FALSE) {
    $dom = XacmlWriter::toDom($xacml);
    $dom->formatOutput = $prettyprint;

    return $dom->saveXML();
  }

  /**
   * Creates the root tag for the base XACML document.
   */
  private static function createRootElement(&$dom, $xacml) {
    $root = $dom->createElementNS(XacmlConstants::xacml, 'Policy');

    //$root->setAttribute('islandora_xacml', '1.0');
    $root->setAttribute('PolicyId','islandora-xacml-editor-v1');
    $root->setAttribute('RuleCombiningAlgId',$xacml['RuleCombiningAlgId']);
    $root->setAttributeNS(XacmlConstants::xmlns, 'xmlns', XacmlConstants::xacml);
    $root->setAttributeNS(XacmlConstants::xmlns, 'xmlns:xsi', XacmlConstants::xsi);

    $dom->appendChild($root);
  }

  /**
   * Creates the empty skeleton document that every XACML document should have. This
   * is the section that says this rule applies to everything. We do all the narrowing
   * of the scope for rules within the rules themselves.
   *
   * @code
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
   */
  private static function createTarget(&$dom, $xacml) {
    $target = $dom->createElementNS(XacmlConstants::xacml,'Target');

    $subject = $dom->createElementNS(XacmlConstants::xacml, 'Subjects');
    $subject->appendChild($dom->createElementNS(XacmlConstants::xacml,'AnySubject'));
    $target->appendChild($subject);

    $resource = $dom->createElementNS(XacmlConstants::xacml, 'Resources');
    $resource->appendChild($dom->createElementNS(XacmlConstants::xacml,'AnyResource'));
    $target->appendChild($resource);

    $action = $dom->createElementNS(XacmlConstants::xacml, 'Actions');
    $action->appendChild($dom->createElementNS(XacmlConstants::xacml,'AnyAction'));
    $target->appendChild($action);

    $dom->documentElement->appendChild($target);
  }

  /**
   * Create all of the <Rule> tags.
   */
  private static function createRules(&$dom, $xacml) {
    foreach($xacml['rules'] as $rule) {
      XacmlWriter::createRule($dom, $rule);
    }
  }

  private static function createRule(&$dom, $rule) {
    $root = $dom->createElementNS(XacmlConstants::xacml, "Rule");
    $root->setAttribute('RuleId', $rule['id']);
    $root->setAttribute('Effect', $rule['effect']);

    XacmlWriter::createRuleTarget($dom, $root, $rule);
    XacmlWriter::createRuleCondition($dom, $root, $rule);

    $dom->documentElement->appendChild($root);
  }

  private static function createRuleTarget(&$dom, &$root, $rule) {
    $target = $dom->createElementNS(XacmlConstants::xacml,"Target");

    XacmlWriter::createRuleTargetSubjects($dom, $target, $rule);
    XacmlWriter::createRuleTargetResources($dom, $target, $rule);
    XacmlWriter::createRuleTargetActions($dom, $target, $rule);

    $root->appendChild($target);
  }

  private static function createRuleTargetSubjects(&$dom, &$target, $rule) {
    $subjects = $dom->createElementNS(XacmlConstants::xacml, "Subjects");
    $subjects->appendChild($dom->createElementNS(XacmlConstants::xacml,"AnySubject"));
    $target->appendChild($subjects);
  }

  private static function createRuleTargetActions(&$dom, &$target, $rule) {
    $actions = $dom->createElementNS(XacmlConstants::xacml, "Actions");
    if(!empty($rule['methods'])) {
      foreach($rule['methods'] as $method) {
        XacmlWriter::createRuleTargetAction($dom, $actions, $method);
      }
    }
    else {
      $actions->appendChild($dom->createElementNS(XacmlConstants::xacml,"AnyAction"));
    }

    $target->appendChild($actions);
  }

  private static function createRuleTargetAction(&$dom, &$actions, $method) {
    $action = $dom->createElementNS(XacmlConstants::xacml,'Action');
    $actionMatch = $dom->createElementNS(XacmlConstants::xacml, 'ActionMatch');
    $actionMatch->setAttribute('MatchId',XacmlConstants::stringequal);
    $action->appendChild($actionMatch);
    $attributeValue = $dom->createElementNS(XacmlConstants::xacml, "AttributeValue", "urn:fedora:names:fedora:2.1:action:id-$method");
    $attributeValue->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
    $actionAttributeDesignator = $dom->createElementNS(XacmlConstants::xacml, "ActionAttributeDesignator");
    $actionAttributeDesignator->setAttribute("AttributeId", "urn:fedora:names:fedora:2.1:action:id");
    $actionAttributeDesignator->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
    $actionMatch->appendChild($attributeValue);
    $actionMatch->appendChild($actionAttributeDesignator);
    $actions->appendChild($action);
  }

  private static function createRuleTargetResources(&$dom, &$target, $rule) {
    $resources = $dom->createElementNS(XacmlConstants::xacml, "Resources");

    if(empty($rule['mimes']) && empty($rule['dsids'])){
      $resources->appendChild($dom->createElementNS(XacmlConstants::xacml,"AnyResource"));
    }
    else {
      foreach($rule['mimes'] as $mime) {
        XacmlWriter::createRuleTargetResource($dom, $resources, $mime, 'mime');
      }
      foreach($rule['dsids'] as $dsid) {
        XacmlWriter::createRuleTargetResource($dom, $resources, $dsid, 'dsid');
      }
    }

    $target->appendChild($resources);
  }

  private static function createRuleTargetResource(&$dom, &$resources, $name, $type) {
    $resource = $dom->createElementNS(XacmlConstants::xacml, 'Resource');
    $resourceMatch = $dom->createElementNS(XacmlConstants::xacml, 'ResourceMatch');
    $resourceMatch->setAttribute('MatchId',XacmlConstants::stringequal);
    $resource->appendChild($resourceMatch);

    $AttributeValue = $dom->createElementNS(XacmlConstants::xacml, 'AttributeValue', $name);
    $AttributeValue->setAttribute('DataType',"http://www.w3.org/2001/XMLSchema#string");

    $ResourceAttributeDesignator = $dom->createElementNS(XacmlConstants::xacml, 'ResourceAttributeDesignator');
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

    $resources->appendChild($resource);
  }

  private static function createRuleCondition(&$dom, &$target, $rule) {
    $condition = $dom->createElementNS(XacmlConstants::xacml,"Condition");
    $condition->setAttribute("FunctionId", "urn:oasis:names:tc:xacml:1.0:function:not");

    if(!empty($rule['users'])) {
      $users = XacmlWriter::createRuleConditionApply($dom, $rule['users'],'user');
    }
    if(!empty($rule['roles'])) {
      $roles = XacmlWriter::createRuleConditionApply($dom, $rule['roles'],'role');
    }

    if(isset($users) && isset($roles)) {
      $apply = $dom->createElementNS(XacmlConstants::xacml, "Apply");
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

    $target->appendChild($condition);
  }

  private static function createRuleConditionApply(&$dom, $attributes, $type) {
    $apply = $dom->createElementNS(XacmlConstants::xacml,'Apply');
    $apply->setAttribute("FunctionId","urn:oasis:names:tc:xacml:1.0:function:string-at-least-one-member-of");

    $subjectAttribureDesignator = $dom->createElementNS(XacmlConstants::xacml, 'SubjectAttributeDesignator');
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

    $stringBag = $dom->createElementNS(XacmlConstants::xacml,"Apply");
    $stringBag->setAttribute("FunctionId","urn:oasis:names:tc:xacml:1.0:function:string-bag");
    $apply->appendChild($stringBag);

    foreach($attributes as $attribute) {
      $attrib = $dom->createElementNS(XacmlConstants::xacml, "AttributeValue", $attribute);
      $attrib->setAttribute("DataType","http://www.w3.org/2001/XMLSchema#string");
      $stringBag->appendChild($attrib);
    }

    return $apply;
  }
}
<?php
/**
 * @file
 * Contains the XacmlParser class.
 */

include_once('XacmlConstants.inc');

/**
 * This static class takes an XACML XML file and parses it into the datastructures used by the XACML class.
 */
class XacmlParser {

  /**
   * This function parses a XML string containing XACML and returns an array of rules representing XACML.
   *
   * @param $xacml_string
   *   String containing the source XACML
   *
   * @return array()
   *   Array containing the XACML represented as datastructures.
   */
  public static function parse ($xacml_string) {
    $rules = array();
    $dom = new DomDocument();
    $dom->preserveWhiteSpace = False;
    $dom->loadXML($xacml_string);
    $xpath = new DomXpath($dom);
    $xpath->registerNamespace('xacml',XacmlConstants::xacml);
    XacmlParser::parseXacml($rules, $dom, $xpath);
    return $rules;
  }

  /**
   * Adds the DSIDs and MIME types specified in the XACML to the datastructure.
   *
   * @param $rule
   *   The rule datastructure to add the ['dsids'] and ['mimes'] arrays to.
   * @param $element
   *   The DOMElement for the <Rule> tag being parsed.
   */
  protected static function findDsidMime(&$rule, $element) {
    $resources = $element->getElementsByTagNameNS(XacmlConstants::xacml, "ResourceMatch");

    foreach($resources as $resource) {
      $value = $resource->childNodes->item(0)->nodeValue;
      $type = $resource->childNodes->item(1)->getAttribute("AttributeId");

      switch($type) {
        case XacmlConstants::mime:
          $rule['mimes'][] = $value;
          break;
        case XacmlConstants::dsid:
          $rule['dsids'][] = $value;
          break;
      }
    }
  }

  /**
   * Adds the methods specified in the XACML to the datastructure.
   *
   * @param $rule
   *   The rule datastructure to add the ['methods'] array to.
   * @param $element
   *   The DOMElement for rhte <Rule> tag being parsed.
   */
  protected static function findMethods(&$rule, $element) {
    $actions = $element->getElementsByTagNameNS(XacmlConstants::xacml, "Actions")->item(0);
    $values = $actions->getElementsByTagNameNS(XacmlConstants::xacml, 'AttributeValue');

    foreach($values as $value) {
      $method = $value->nodeValue;

      // methods are returned as they are represented in XACML
      // for example: urn:fedora:names:fedora:2.1:action:id-getDatastreamDissemination
      // here we split off the first 38 characters, which for this example would
      // give us: getDatastreamDissemination
      $rule['methods'][] = substr($method,38);
    }
  }

  /**
   * Uses XPath to add the specified roles to the XACML datastructure.
   *
   * @param $rule
   *   The rule to add the ['roles'] array to.
   * @param $element
   *   The <Rule> DOMElement.
   * @param $xpath
   *   An DOMXPath class instantiated for this DOMDocument.
   */
  protected static function findRoles(&$rule, $element, $xpath) {
    $role_designator = $xpath->query('.//xacml:Apply[@FunctionId="' . XacmlConstants::onememeberof .
      '"]/xacml:SubjectAttributeDesignator[@AttributeId="fedoraRole"]',$element);
    $role_attrib = $xpath->query('../xacml:Apply/xacml:AttributeValue',$role_designator->item(0));

    foreach($role_attrib as $role) {
      $rule['roles'][] = $role->nodeValue;
    }
  }

  /**
   * Uses XPath to add the specified users to the XACML datastructure.
   *
   * @param $rule
   *   The rule to add the ['users'] array to.
   * @param $element
   *   The <Rule> DOMElement.
   * @param $xpath
   *   An DOMXPath class instantiated for this DOMDocument.
   */
  protected static function findUsers(&$rule, $element, $xpath) {
    $user_designator = $xpath->query('.//xacml:Apply[@FunctionId="' . XacmlConstants::onememeberof .
      '"]/xacml:SubjectAttributeDesignator[@AttributeId="' . XacmlConstants::loginid . '"]',$element);
    $user_attrib = $xpath->query('../xacml:Apply/xacml:AttributeValue',$user_designator->item(0));

    foreach($user_attrib as $user) {
      $rule['users'][] = $user->nodeValue;
    }
  }

  /**
   * Parses the passed in Xacml returning the $rules array containing the datastructure for
   * each rule.
   *
   * @param $rules
   *   The rules array to be populated by this function.
   * @param $dom
   *   The DOMDocument containing the XACML.
   * @param $xpath
   *   An instantianted DOMXPath class for the above DOMDomcument.
   */
  protected static function parseXacml(&$rules, $dom, $xpath){
    # get each rule element
    foreach($dom->getElementsByTagNameNS(XacmlConstants::xacml,"Rule") as $rule_element) {
      $rule = array();

      $rule['id'] = $rule_element->getAttribute("RuleId");
      $rule['effect'] = $rule_element->getAttribute("Effect");

      $rule['dsids'] = array();
      $rule['mimes'] = array();
      $rule['methods'] = array();
      $rule['users'] = array();
      $rule['roles'] = array();

      XacmlParser::findDsidMime($rule, $rule_element);
      XacmlParser::findMethods($rule, $rule_element);
      XacmlParser::findRoles($rule, $rule_element, $xpath);
      XacmlParser::findUsers($rule, $rule_element, $xpath);

      $rules[] = $rule;
    }
  }

}
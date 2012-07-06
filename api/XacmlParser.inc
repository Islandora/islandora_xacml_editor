<?php

/**
 * @file
 * Contains the XacmlParser class.
 */

include_once 'XacmlConstants.inc';
include_once 'XacmlException.inc';

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
   * @throws XacmlException
   *   Thrown when the passed in policy can't be parsed. Particularly when the policy was not generated
   *   by the islandora XACML editor.
   *
   * @return array()
   *   Array containing the XACML represented as datastructures.
   */
  public static function parse($xacml_string) {
    $xacml = array();
    $xacml['rules'] = array();

    // do some funky workaround stuff in order to get loadXML to throw an exception
    // instead of outputting a warning then failing in an ugly way later on

    $dom = new DomDocument();
    $dom->preserveWhiteSpace = FALSE;

    // Throw exception if DomDocument gave us a Parse error
    if ($dom->loadXML($xacml_string) == FALSE) {
      throw new XacmlException('Error Parsing XML.');
    }

    // Do basic sanity check that root element is <Policy>
    if ($dom->documentElement->tagName != 'Policy')
      throw new XacmlException('Root tag is not Policy.');

    // see if the policy was written by islandora, othewise throw an exception
    if (!$dom->documentElement->hasAttribute('PolicyId') || $dom->documentElement->getAttribute("PolicyId") != 'islandora-xacml-editor-v1')
      throw new XacmlException('XACML file was not written by XACML Editor.');

    $xpath = new DomXpath($dom);
    $xpath->registerNamespace('xacml', XacmlConstants::XACML);
    XacmlParser::parseXacml($xacml, $dom, $xpath);

    return $xacml;
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
    $resources = $element->getElementsByTagNameNS(XacmlConstants::XACML, "ResourceMatch");

    foreach ($resources as $resource) {
      $value = $resource->childNodes->item(0)->nodeValue;
      $type = $resource->childNodes->item(1)->getAttribute("AttributeId");

      switch ($type) {
        case XacmlConstants::MIME:
          $rule['mimes'][] = $value;
          break;

        case XacmlConstants::DSID:
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
   *   The DOMElement for the <Rule> tag being parsed.
   */
  protected static function findMethods(&$rule, $element) {
    $actions = $element->getElementsByTagNameNS(XacmlConstants::XACML, "Actions")->item(0);
    $values = $actions->getElementsByTagNameNS(XacmlConstants::XACML, 'AttributeValue');

    foreach ($values as $value) {
      $method = $value->nodeValue;

      if (preg_match('/api-a/', $method) || preg_match('/api-m/', $method)) {
        $rule['methods'][] = drupal_substr($method, 35);
      }
      else {
        // methods are returned as they are represented in XACML
        // for example: urn:fedora:names:fedora:2.1:action:id-getDatastreamDissemination
        // here we split off the first 38 characters, which for this example would
        // give us: getDatastreamDissemination
        $rule['methods'][] = drupal_substr($method, 38);
      }
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
    $role_designator = $xpath->query('.//xacml:Apply[@FunctionId="' . XacmlConstants::ONEMEMBEROF .
      '"]/xacml:SubjectAttributeDesignator[@AttributeId="fedoraRole"]', $element);

    if ($role_designator->length != 0) {
      $role_attrib = $xpath->query('../xacml:Apply/xacml:AttributeValue', $role_designator->item(0));

      foreach ($role_attrib as $role) {
        $rule['roles'][] = $role->nodeValue;
      }
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
    $user_designator = $xpath->query('.//xacml:Apply[@FunctionId="' . XacmlConstants::ONEMEMBEROF .
      '"]/xacml:SubjectAttributeDesignator[@AttributeId="' . XacmlConstants::LOGINID . '"]', $element);

    if ($user_designator->length != 0) {
      $user_attrib = $xpath->query('../xacml:Apply/xacml:AttributeValue', $user_designator->item(0));

      foreach ($user_attrib as $user) {
        $rule['users'][] = $user->nodeValue;
      }
    }
  }

  /**
   * Parses the passed in Xacml returning the $xacml array.
   *
   * @param $xacml
   *   The xacml rules array to be populated by this function.
   * @param $dom
   *   The DOMDocument containing the XACML.
   * @param $xpath
   *   An instantianted DOMXPath class for the above DOMDomcument.
   */
  protected static function parseXacml(&$xacml, $dom, $xpath) {
    $xacml['PolicyId'] = $dom->documentElement->getAttribute("PolicyId");
    //$xacml['EditorVersion'] = $dom->documentElement->getAttribute("islandora_xacml");
    $xacml['RuleCombiningAlgId'] = $dom->documentElement->getAttribute("RuleCombiningAlgId");

    // get each rule element
    foreach ($dom->getElementsByTagNameNS(XacmlConstants::XACML, "Rule") as $rule_element) {
      $rule = array();

      $rule['effect'] = $rule_element->getAttribute("Effect");
      $rule['ruleid'] = $rule_element->getAttribute("RuleId");
      $rule['dsids'] = array();
      $rule['mimes'] = array();
      $rule['methods'] = array();
      $rule['users'] = array();
      $rule['roles'] = array();

      if ($rule['effect'] == 'Deny') {
        XacmlParser::findDsidMime($rule, $rule_element);
        XacmlParser::findMethods($rule, $rule_element);
        XacmlParser::findRoles($rule, $rule_element, $xpath);
        XacmlParser::findUsers($rule, $rule_element, $xpath);
      }

      $xacml['rules'][] = $rule;
    }
  }

}

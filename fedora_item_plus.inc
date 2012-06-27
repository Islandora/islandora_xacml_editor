<?php

class Fedora_Item_Plus extends Fedora_Item {

  /**
   * Create an object to represent an item in the Fedora repository.
   * Throws a SOAPException if the PID is not in the repository.
   *
   * @param string $pid
   * @return Fedora_Item_Plus
   */
	
  function __construct($pid) {
		parent::__construct($pid);
	}
	
  /**
   * Currently the Fedora API call getRelationships is reporting an uncaught
   * exception so we will parse the RELS-EXT ourselves and simulate the
   * documented behaviour.
   * @param String $relationship - filter the results to match this string.
   */
  function get_relationships($relationship = NULL, $namespace_uri = NULL) {
		if (empty($namespace_uri)) {
			$namespace_uri = 'info:fedora/fedora-system:def/relations-external#';
		}
    $relationships = array();
    try {
      $relsext = $this->get_datastream_dissemination('RELS-EXT');
    } catch (exception $e) {
      drupal_set_message(t("Error retrieving RELS-EXT of object $pid"), 'error');
      return $relationships;
    }

    // Parse the RELS-EXT into an associative array.
    $relsextxml = new DOMDocument();
    $relsextxml->loadXML($relsext);
    $relsextxml->normalizeDocument();
    $rels = $relsextxml->getElementsByTagNameNS($namespace_uri, '*');
    foreach ($rels as $child) {
      if (empty($relationship) || preg_match("/$relationship/", $child->tagName)) {
        $relationships[] = array(
          'subject' => $this->pid,
          'predicate' => $child->tagName,
          'object' => substr($child->getAttributeNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'resource'), 12),
        );
      }
    }

    return $relationships;
  }
  
  function purge_relationships($relationship, $namespaceURI = NULL) {
    $relsext = $this->get_datastream_dissemination('RELS-EXT');
    if (empty($namespaceURI)) {
      $namespaceURI = 'info:fedora/fedora-system:def/relations-external#';
    }
    // Pre-pend a namespace prefix to recognized relationships

    switch ($relationship) {
      case 'rel:isMemberOf':
      case 'fedora:isMemberOf':
        $relationship = "isMemberOf";
        $namespaceURI = 'info:fedora/fedora-system:def/relations-external#';
        break;
      case "rel:isMemberOfCollection":
      case "fedora:isMemberOfCollection":
        $relationship = "isMemberOfCollection";
        $namespaceURI = 'info:fedora/fedora-system:def/relations-external#';
        break;
      case "fedora:isPartOf":
        $relationship = "isPartOf";
        $namespaceURI = 'info:fedora/fedora-system:def/relations-external#';
        break;
      case "rel:hasModel":
      case "hasModel":
        $relationship = "hasModel";
        $namespaceURI = FEDORA_MODEL_URI;
        break;
    }

    $relsextxml = new DomDocument();
    $relsextxml->loadXML($relsext);
    $rels = $relsextxml->getElementsByTagNameNS($namespaceURI, $relationship);
    $elements_to_remove = array();
    $modified = FALSE;
    if (!empty($rels)) {
      foreach ($rels as $rel) {
        $elements_to_remove[] = $rel;
        $modified = TRUE;
      }
      foreach($elements_to_remove as $element) {
        $element->parentNode->removeChild($element);
      }
    }
    if ($modified) {
      $this->modify_datastream_by_value($relsextxml->saveXML(), 'RELS-EXT', "Fedora Object-to-Object Relationship Metadata", 'text/xml');
    }
    return $modified;
  }

}

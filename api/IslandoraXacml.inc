<?php

module_load_include('inc', 'islandora_xacml_api', 'Xacml');

/**
 * Subclass Xacml to facilitate communication to Islandora/Fedora.
 * 
 * Need to have Drupal bootstrapped, due to use of module_load_includes
 */
class IslandoraXacml extends Xacml {
  /**
   * The idenitfy the datastream from which this policy was obtained (and to
   * which it should be written back to).
   *
   * @var $item
   * @var $pid
   * @var $dsid
   */
  protected $item, $pid, $dsid;
  
  function __construct($pid, $dsid = 'POLICY') {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $item = new Fedora_Item($pid);
    
    $xacml = NULL;
    if (isset($item->datastreams[$dsid])) {
      $xacml = $item->get_datastream_dissemination($dsid);
    }
    
    parent::_construct($xacml);
    $this->pid = $pid;
    $this->dsid = $dsid;
    $this->item = $item;
  }

  function writeBackToFedora() {
    $xml = $this->getXmlString();
    if (array_key_exists($this->dsid, $this->item->datastreams)) {
      return $this->item->modify_datastream($xml, $this->dsid, 'Xacml Policy Stream', 'text/xml');
    }
    else {
      return $this->item->add_datastream_from_string($xml, $this->dsid, 'Xacml Policy Stream', 'text/xml', 'X');
    }
  }
}


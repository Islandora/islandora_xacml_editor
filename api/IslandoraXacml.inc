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
    
    parent::__construct($xacml);
    $this->pid = $pid;
    $this->dsid = $dsid;
    $this->item = $item;
  }

  function writeBackToFedora() {
    $xml = $this->getXmlString();
    if (array_key_exists($this->dsid, $this->item->datastreams)) {
      $this->item->modify_datastream($xml, $this->dsid, 'Xacml Policy Stream', 'text/xml');
    }
    else {
      $this->item->add_datastream_from_string($xml, $this->dsid, 'Xacml Policy Stream', 'text/xml', 'X');
    }

    $viewable_by_user = 'isViewableByUser';
    $viewable_by_role = 'isViewableByRole';
    // remove all old policy related rels-ext statements

    if (variable_get('islandora_xacml_editor_save_relationships', TRUE)) {
      $this->item->purge_relationships($viewable_by_user, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      $this->item->purge_relationships($viewable_by_role, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      foreach (array_keys($this->item->datastreams) as $dsid) {
        $this->item->purge_dsid_relationships($dsid, $viewable_by_user, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
        $this->item->purge_dsid_relationships($dsid, $viewable_by_role, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      }

      if ($this->viewingRule->isPopulated()) {
        // recompute the new values from the policy
        foreach ($this->viewingRule->getValues('users') as $account) {
          $this->item->add_relationship($viewable_by_user, $account, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
        }
        foreach ($this->viewingRule->getValues('roles') as $role) {
          $this->item->add_relationship($viewable_by_role, $role, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
        }
      }

      if ($this->datastreamRule->isPopulated()) {
        foreach ($this->datastreamRule->getValues('dsid') as $dsid) {
          // recompute the new values from the policy
          foreach ($this->datastreamRule->getValues('users') as $account) {
            $this->item->add_dsid_relationship($dsid, $viewable_by_user, $account, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
          }
          foreach ($this->datastreamRule->getValues('roles') as $role) {
            $this->item->add_dsid_relationship($dsid, $viewable_by_role, $role, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
          }
        } 
      }
    }
  }
}


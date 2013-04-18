<?php

/**
 * @file
 * Subclass Xacml to facilitate communication to Islandora/Fedora.
 *
 * Need to have Drupal bootstrapped, due to use of module_load_includes
 */

class IslandoraXacml extends Xacml {
  /**
   * The idenitfy the datastream from which this policy was obtained (and to
   * which it should be written back to).
   *
   * @var $object
   * @var $pid
   */
  protected $object;
  protected $pid;

  /**
   * Constructor.
   *
   * @param FedoraObject $object
   *   A FedoraObject.
   * @param mixed $xacml
   *   A string containing XACML XML, or NULL to attempt to load from the given
   *   PID and DSID.
   */
  public function __construct($object, $xacml = NULL) {
    if ($xacml === NULL && isset($object['POLICY'])) {
      $xacml = $object['POLICY']->content;
    }

    parent::__construct($xacml);
    $this->pid = $object->id;
    $this->object = $object;
  }

  /**
   * Writes our XACML stream to our PID/DSID pair...
   */
  public function writeBackToFedora() {
    // Only add relationships on POLICY datastream.
    if (variable_get('islandora_xacml_api_save_relationships', TRUE)) {
      $this->writeRelations();
    }

    $xml = $this->getXmlString();
    if (isset($this->object['POLICY'])) {
      $this->object['POLICY']->content = $xml;
    }
    else {
      $xacml_datastream = $this->object->constructDatastream('POLICY', 'X');
      $xacml_datastream->label = 'XACML Policy Stream';
      $xacml_datastream->mimetype = 'text/xml';
      $xacml_datastream->setContentFromString($xml);
      $this->object->ingestDatastream($xacml_datastream);
    }

    return TRUE;
  }

  /**
   * Writes our relations to Fedora.
   */
  protected function writeRelations() {
    $viewable_by_user = 'isViewableByUser';
    $viewable_by_role = 'isViewableByRole';
    $this->object->relationships->remove(ISLANDORA_RELS_EXT_URI, $viewable_by_user);
    $this->object->relationships->remove(ISLANDORA_RELS_EXT_URI, $viewable_by_role);

    foreach ($this->object as $dsid => $value) {
      $this->object[$dsid]->relationships->remove(ISLANDORA_RELS_INT_URI, $viewable_by_user);
      $this->object[$dsid]->relationships->remove(ISLANDORA_RELS_INT_URI, $viewable_by_role);
    }

    if ($this->viewingRule->isPopulated()) {
      // Recompute the new values from the policy.
      $view_users = $this->viewingRule->getUsers();
      $view_roles = $this->viewingRule->getRoles();
      if (count($view_users) > 0) {
        foreach ($view_users as $user) {
          $this->object->relationships->add(ISLANDORA_RELS_EXT_URI, $viewable_by_user, $user, TRUE);
        }
      }
      if (count($view_roles) > 0) {
        foreach ($view_roles as $role) {
          $this->object->relationships->add(ISLANDORA_RELS_EXT_URI, $viewable_by_role, $role, TRUE);
        }
      }
    }

    if ($this->datastreamRule->isPopulated()) {
      foreach ($this->datastreamRule->getDsids() as $dsid) {
        $ds_users = $this->datastreamRule->getUsers();
        $ds_roles = $this->datastreamRule->getRoles();
        if (count($ds_users) > 0) {
          foreach ($ds_users as $user) {
            if (isset($this->object[$dsid])) {
              $this->object[$dsid]->relationships->add(ISLANDORA_RELS_INT_URI, $viewable_by_user, $user, TRUE);
            }
          }
        }
        if (count($ds_roles) > 0) {
          foreach ($ds_roles as $role) {
            if (isset($this->object[$dsid])) {
              $this->object[$dsid]->relationships->add(ISLANDORA_RELS_INT_URI, $viewable_by_role, $role, TRUE);
            }
          }
        }
      }
    }
  }
}

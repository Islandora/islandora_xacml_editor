<?php

/**
 * @file Subclass Xacml to facilitate communication to Islandora/Fedora.
 *
 * Need to have Drupal bootstrapped, due to use of module_load_includes
 */


module_load_include('inc', 'islandora_xacml_api', 'Xacml');

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

  /**
   * Constructor.
   *
   * @param $pid
   *   A string containing a Fedora PID, from which to attempt to load and
   *   write back to.
   * @param $dsid
   *   A string containing a Fedora DSID, from which to attempt to load and
   *   write back to.
   * @param $xacml
   *   A string containing XACML XML, or NULL to attempt to load from the given
   *   PID and DSID.
   */
  function __construct($pid, $dsid = 'POLICY', $xacml = NULL) {
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');

    $item = islandora_object_load($pid);

    if ($xacml === NULL && isset($item[$dsid])) {
      $xacml = $item->repository->api->a->getDatastreamDissemination($pid, $dsid);
    }

    parent::__construct($xacml);
    $this->pid = $pid;
    $this->dsid = $dsid;
    $this->item = $item;
  }

  /**
   * Writes our XACML stream to our PID/DSID pair...
   */
  function writeBackToFedora() {
    //Only add relationships on POLICY datastream, as only the POLICY is enforced.
    if ($this->dsid == 'POLICY' && variable_get('islandora_xacml_api_save_relationships', TRUE)) {
      $this->writeRelations();
    }

    $xml = $this->getXmlString();
    if (isset($this->item[$this->dsid])) {
      $params = array(
        'dsString' => $xml,
      );
      $this->item->repository->api->m->modifyDatastream($this->pid, $this->dsid, $params);
    }
    else {
      $xacml_datastream = $this->item->constructDatastream($this->dsid, 'X');
      $xacml_datastream->label = 'XACML Policy Stream';
      $xacml_datastream->mimetype = 'text/xml';
      $xacml_datastream->setContentFromString($xml);
      $this->item->ingestDatastream($xacml_datastream);
    }

    return TRUE;
  }

  protected function writeRelations() {
    $viewable_by_user = 'isViewableByUser';
    $viewable_by_role = 'isViewableByRole';

    $this->item->relationships->remove(ISLANDORA_RELS_EXT_URI, $viewable_by_user);
    $this->item->relationships->remove(ISLANDORA_RELS_EXT_URI, $viewable_by_role);

    $ds_array = $this->item->repository->api->a->listDatastreams($this->pid);

    foreach ($ds_array as $dsid => $value) {
      $this->item[$dsid]->relationships->remove(ISLANDORA_RELS_INT_URI, $viewable_by_user);
      $this->item[$dsid]->relationships->remove(ISLANDORA_RELS_INT_URI, $viewable_by_role);
    }

    if ($this->viewingRule->isPopulated()) {
      // recompute the new values from the policy
      $view_users = $this->viewingRule->getUsers();
      $view_roles = $this->viewingRule->getRoles();
      if (count($view_users) > 0) {
        foreach ($view_users as $user) {
          $this->item->relationships->add(ISLANDORA_RELS_EXT_URI, $viewable_by_user, $user, TRUE);
        }
      }
      if (count($view_roles) > 0) {
        foreach ($view_roles as $role) {
          $this->item->relationships->add(ISLANDORA_RELS_EXT_URI, $viewable_by_role, $role, TRUE);
        }
      }
    }

    if ($this->datastreamRule->isPopulated()) {
      foreach ($this->datastreamRule->getDsids() as $dsid) {
        $ds_users = $this->datastreamRule->getUsers();
        $ds_roles = $this->datastreamRule->getRoles();
        if (count($ds_users) > 0) {
          foreach ($ds_users as $user) {
            if (isset($this->item[$dsid])) {
              $this->item[$dsid]->relationships->add(ISLANDORA_RELS_INT_URI, $viewable_by_user, $user, TRUE);
            }
          }
        }
        if (count($ds_roles) > 0) {
          foreach ($ds_roles as $role) {
            if (isset($this->item[$dsid])) {
              $this->item[$dsid]->relationships->add(ISLANDORA_RELS_INT_URI, $viewable_by_role, $role, TRUE);
            }
          }
        }
      }
    }
  }
}


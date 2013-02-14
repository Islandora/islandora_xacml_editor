---- Fedora configuration ----

It may be desirable--and in fact necessary for some modules--to disable/remove
one of the default XACML policies which denies any interactions with the
POLICY datastream to users without the "administrator" role.

This policy is located here:
$FEDORA_HOME/data/fedora-xacml-policies/repository-policies/default/deny-policy-management-if-not-administrator.xml

---- Solr searching hook ----

In order to comply with XACML restrictions placed on objects, a hook is used to filter results that do not conform to a searching user's roles and name.
This hook will not function correctly if the Solr fields for 'ViewableByUser' and 'ViewableByRole' are not defined correctly as they are set in the XSLT.
These values can be set through the admin page for the module.


---- CHILD_SECURITY notes ----

Currently, it is not possible to implement datastream specific restrictions on children objects where the target datastream does not already exist.
For target objects where the datastream does not exist the respective filter(s) for the non-existent datastreams will be ignored and not wrote
in the RELS-INT
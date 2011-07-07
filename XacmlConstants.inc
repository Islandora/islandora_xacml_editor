<?php
/**
 * @file
 * This file contains a class that specifies some constants that are used through the Islandora Xacml Module.
 */

/**
 * Helper class specifying some useful constants.
 */
class XacmlConstants {
  const xacml = "urn:oasis:names:tc:xacml:1.0:policy";
  const mime = "urn:fedora:names:fedora:2.1:resource:datastream:mimeType";
  const dsid = "urn:fedora:names:fedora:2.1:resource:datastream:id";
  const onememeberof = "urn:oasis:names:tc:xacml:1.0:function:string-at-least-one-member-of";
  const loginid = "urn:fedora:names:fedora:2.1:subject:loginId";
  const xsi = 'http://www.w3.org/2001/XMLSchema-instance';
  const xmlns = 'http://www.w3.org/2000/xmlns/';
}
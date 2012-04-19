<?php
/**
 * @file
 * Definition of .
 */

class RegisterExport {
  protected $id = NULL;

  protected $name;
  protected $type;

  protected $title = NULL;

  protected $fields = array();
  protected $groups = array();
  protected $options = array();

  protected $export_module = NULL;
  protected $export_type = NULL;
  protected $export_type_title = NULL;
  protected $export_in_code_only = NULL;
  protected $export_table = NULL;
  protected $export_api_version = NULL;

  public function __construct(&$object) {
    if (isset($object->id)) {
      $this->id = $object->id;
    }

    $this->name = $object->register_name;

    $this->type = $object->register_type;


    // Config variables.

    $config = isset($object->register_config) ? $object->register_config : array();

    if (isset($config['groups'])) {
      $this->fields = $config['groups'];
    }

    if (isset($config['fields'])) {
      $this->fields = $config['fields'];
    }

    if (isset($config['title'])) {
      $this->title = $config['title'];
    }

    if (isset($config['options'])) {
      $this->options = $config['options'];
    }

    // Export variables.

    if (isset($object->export_module)) {
      $this->export_module = $object->export_module;
    }

    if (isset($object->export_type)) {
      $this->export_type = $object->export_type;
    }

    if (isset($object->type)) {
      $this->export_type_title = $object->type;
    }

    if (isset($object->in_code_only)) {
      $this->export_in_code_only = $object->in_code_only;
    }

    if (isset($object->table)) {
      $this->export_table = $object->table;
    }

    if (isset($object->api_version)) {
      $this->export_api_version = $object->api_version;
    }
  }

  public function buildExportObject() {
    $obj = new stdClass();

    $obj->id = $this->id;

    $obj->register_name = $this->name;
    $obj->register_type = $this->type;

    $config = array();
    $config['fields'] = $this->fields;
    $config['groups'] = $this->groups;
    $config['options'] = $this->options;
    $obj->register_config = $config;

    $obj->export_type = $this->export_type = EXPORT_IN_DATABASE;

    return $obj;
  }

  public function buildRegisterConfig() {
    $config = array();
    foreach ($this->fields as $field => $info) {
      $field_config = array();

      $field_config['type'] = $info['type'];

      $size = $info['size_preset'] ? $info['size_preset'] : $info['size'];
      if (in_array($field_config['type'], register_field_types_numeric())) {
        $field_config['size'] = $size;
      }
      else {
        $field_config['length'] = $size;
      }

      $field_config['not null'] = isset($info['nullable']) ? !$info['nullable'] : TRUE;

      if (isset($info['default_type'])) {
        if ($info['default_type'] == 'null') {
          $field_config['default'] = NULL;
        }
        elseif ($info['default_type'] == 'value') {
          $field_config['default'] = $info['default_value'];
        }
      }

      if (!isset($config[$info['group']])) {
        $config[$info['group']] = array();
      }
      $config[$info['group']][$field] = $field_config;
    }

    $config['options'] = $this->options;

    return $config;
  }

  public function updateField($field, $info) {
    if (isset($this->fields[$field])) {
      $weight = isset($this->fields[$field]['weight']) ? $this->fields[$field]['weight'] : 0;
      $this->fields[$field] = $info + array('weight' => $weight);
    }
  }

  public function removeFields() {
    $this->fields = array();
    $this->groups = array();
  }

  public function removeOptions() {
    $this->options = array();
  }

  public function setOption($name, $value) {
    $this->options[$name] = $value;
  }

  public function getOption($name, $default = NULL) {
    return isset($this->options[$name]) ? $this->options[$name] : $default;
  }

  public function addField($field, $info) {
    $this->fields[$field] = $info;
    $group = $info['group'];
    if (!isset($this->groups[$group])) {
      $this->groups[$group] = array();
    }
    if (!array_search($field, $this->groups[$group])) {
      $this->groups[$group][] = $field;
    }
  }

  public function removeField($field) {
    unset($this->fields[$field]);
    foreach ($this->groups as $group => $info) {
      $index = array_search($field, $info);
      if ($index !== FALSE) {
        unset($this->groups[$group][$index]);
      }
    }
  }

  public function getFields() {
    $fields = array();
    $weight = array();
    foreach ($this->fields as $field => $info) {
      $fields[$field] = $info;
      $weight[] = isset($info['weight']) ? $info['weight'] : (count($this->fields) + 1);
    }
    array_multisort($weight, SORT_ASC, $fields, SORT_REGULAR);
    return $fields;
  }

  public function getField($field) {
    return isset($this->fields[$field]) ? $this->fields[$field] : NULL;
  }

  public function reorderFields($fields) {
    foreach ($fields as $field => $weight) {
      if (isset($this->fields[$field])) {
        $this->fields[$field]['weight'] = (int) $weight;
      }
    }
  }

  public function setDescription($description) {
    $this->description = $description;
  }

  public function getDescription() {
    return $this->description;
  }

  public function getName() {
    return $this->name;
  }

//  public function setType($type) {
//    $this->type = $type;
//  }

  public function getType() {
    return $this->type;
  }

  public function getExportType() {
    return $this->export_type;
  }

  public function setExportType($type) {
    if (in_array($type, array(EXPORT_IN_CODE, EXPORT_IN_DATABASE))) {
      $this->export_type = $type;
    }
  }

  public function getId() {
    return $this->id;
  }

  public function getExportTypeTitle() {
    return $this->export_type_title;
  }

  public function getTitle() {
    return $this->title;
  }

  public function isExportInCodeOnly() {
    return (bool) $this->export_in_code_only;
  }

}

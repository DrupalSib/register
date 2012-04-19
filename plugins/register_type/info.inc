<?php

$plugin = array(
  'title' => t('Info register'),
  'handler' => array(
    'class' => 'RegisterTypeInfo',
    'parent' => 'RegisterType',
  ),
);

class RegisterTypeInfo extends RegisterType {

  public function init($config) {
    parent::init($config);

    $this->schema['tables']['info'] = array(
      'fields' => $this->getField(array('#dimensions', '#resources', '#details')),
      'primary key' => $this->getFieldName('#dimensions'),
    );
  }

  public function setValue($data, $options = array()) {
    $result = NULL;
    $exist = isset($options['exist']) ? $options['exist'] : $this->readRecord('info', $data);
    // Update record.
    if ($exist) {
      $query = db_update($this->getDbTable('info'));
      foreach($this->getFieldValue('@primary', 'info', $data) as $field => $value) {
        $query->condition($field, $value);
      }
      $result = $query->fields($this->getFieldValue('#resources', 'info', $data))->execute();
    }
    // Insert record.
    else {
      $result = db_insert($this->getDbTable('info'))
          ->fields($this->getFieldValue('@all', 'info', $data))
          ->execute();
    }
    return $result;
  }

  public function getValue($data) {
    return $this->readRecord('info', $data, $this->getFieldName('@not-primary', 'info'));
  }

  public function getRecords($filter) {
    $query = db_select($this->getDbTable('info'), 't');
    $query->fields('t');
    foreach ($this->getFieldValueOperation('@not-primary', 'info', $filter, FALSE, FALSE) as $field => $value) {
      $query->condition("t.$field", $this->fieldGetValue($value), $this->fieldGetOperator($value));
    }
    $result = $query->execute();
    $records = array();
    while ($record = $result->fetchAssoc()) {
      $records[] = $record;
    }
    return $records;
  }
}

<?php

$plugin = array(
  'title' => t('History register'),
  'handler' => array(
    'class' => 'RegisterTypeHistory',
    'parent' => 'RegisterType',
  ),
);

class RegisterTypeHistory extends RegisterType {

  public function init($config) {
    parent::init($config);

    $this->schema['fields'] = array_merge(
      $this->schema['fields'],
      array(
        'date' => array('type' => 'int', 'size' => 'normal', 'unsigned' => TRUE, 'not null' => TRUE),
      )
    );

    $this->schema['tables']['info'] = array(
      'fields' => $this->getField(array('#dimensions', '#resources', '#details')),
      'primary key' => $this->getFieldName('#dimensions'),
    );

    $this->schema['tables']['history'] = array(
      'fields' => $this->getField(array('date', '#dimensions', '#resources')),
      'indexes' => array(
        'dimension' => $this->getFieldName('#dimensions'),
      ),
      'primary key' => $this->getFieldName(array('date', '#dimensions')),
    );
  }

  public function setValue($data) {
    $this->writeRecord('info', $data);
    $this->writeRecord('history', $data);
  }

  protected function writeRecord($table, $data, $options = array()) {
    $result = NULL;
    $exist = isset($options['exist']) ? $options['exist'] : $this->readRecord($table, $data);
    // Update record.
    if ($exist) {
      $query = db_update($this->getDbTable($table));
      foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
        $query->condition($field, $value);
      }
      $result = $query->fields($this->getFieldValue($table == 'info' ? '#resources' : '@not-primary', $table, $data))
          ->execute();
    }
    // Insert record.
    else {
      $result = db_insert($this->getDbTable($table))
          ->fields($this->getFieldValue('@all', $table, $data))
          ->execute();
    }
    return $result;
  }

  public function getValue($data) {
    $result = NULL;
    if (isset($data['date'])) {
      $result = $this->readRecordHistory('history', $data);
    }
    else {
      $result = $this->readRecord('info', $data, $this->getFieldName('#resources', 'info'));
    }
    return $result;
  }

  protected function readRecordHistory($table, $data) {
    $query = db_select($this->getDbTable($table), 't');
    $query->fields('t', $this->getFieldName('#resources', $table));
    foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
      if ($field == 'date') {
        $query->condition('t.date', $value, '<=');
      }
      else {
        $query->condition($field, $value);
      }
    }
    $query->orderBy('date', 'DESC');
    $query->range(0, 1);
    $result = $query->execute()->fetchAssoc();
    return $result;
  }
}

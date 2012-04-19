<?php

abstract class RegisterType {
  protected $schema = array();
  private $name;

  public function __construct($name) {
    $this->name = $name;
  }

  /**
   * @return string
   */
  public function getName() {
    return $this->name;
  }

  public function init($config) {
    $dim = isset($config['dimensions']) ? $config['dimensions'] : array();
    $res = isset($config['resources']) ? $config['resources'] : array();
    $det = isset($config['details']) ? $config['details'] : array();

    $this->schema = array(
      'fields' => array(),
      'groups' => array(),
      'tables' => array(),
    );

    $this->schema['fields'] = array_merge($dim, $res, $det);

    $this->schema['groups']['dimensions'] = array_keys($dim);
    $this->schema['groups']['resources'] = array_keys($res);
    $this->schema['groups']['details'] = array_keys($det);
  }

  public function getDbSchema() {
    global $db_type;
    $result = array();
    foreach($this->schema['tables'] as $name => $table) {
      $result[$this->getDbTable($name)] = $table;
      if (in_array($db_type, array('mysql', 'mysqli'))) {
        $result[$this->getDbTable($name)]['mysql_suffix'] = ' ENGINE=INNODB ';
      }
    }
    return $result;
  }

  protected function getTable($table) {
    if (!isset($this->schema['tables'][$table])) {
      throw new Exception('Invalid table name');
    }
    return $this->schema['tables'][$table];
  }

  protected function getDbTable($name) {
    if (!isset($this->schema['tables'][$name])) {
      throw new Exception('Invalid table name');
    }
    return 'r_' . $this->getName() . '_' . $name;
  }

  protected function getField($filter, $table = NULL) {
    $result = array();
    if ($table) {
      $fields = $this->schema['tables'][$table]['fields'];
    }
    else {
      $fields = $this->schema['fields'];
    }
    foreach (is_array($filter) ? $filter : array($filter) as $value) {
      if (is_string($value)) {
        $meta = substr($value, 0, 1);
        if ($meta == '#') {
          foreach ($this->schema['groups'][substr($value, 1)] as $field) {
            if (isset($fields[$field])) {
              $result[$field] = $fields[$field];
            }
          }
        }
        elseif ($meta == '@') {
          if ($table) {
            switch (substr($value, 1)) {
              case 'all':
                $result = array_merge($result, $this->getField(array_keys($this->schema['tables'][$table]['fields'])));
                break;
              case 'primary':
                $result = array_merge($result, $this->getField($this->schema['tables'][$table]['primary key']));
                break;
              case 'not-primary':
                $result = array_merge(
                  $result,
                  array_diff_key(
                    $this->getField(array_keys($this->schema['tables'][$table]['fields'])),
                    $this->getField($this->schema['tables'][$table]['primary key'])
                  )
                );
                break;
            }
          }
        }
        else {
          if (isset($fields[$value])) {
            $result[$value] = $fields[$value];
          }
        }
      }
    }
    return $result;
  }

  protected function getFieldValueOperation($filter, $table, $data, $single = FALSE, $required = TRUE) {
    $result = array();
    $values = array();
    $operators = array();
    foreach ($data as $field => $value) {
      $values[$field] = $this->fieldGetValue($value);
      $operators[$field] = $this->fieldGetOperator($value);
    }
    foreach ($this->getFieldValue($filter, $table, $values, FALSE, $required) as $field => $value) {
      $result[$field] = array($value, $operators[$field]);
    }
    if ($single) {
      return array_shift($result);
    }
    else {
      return $result;
    }
  }

  /**
   * @throws Exception
   * @param $filter
   * @param $table
   * @param $data
   * @param bool $single
   * @param bool $required
   * @return array|mixed|null
   */
  protected function getFieldValue($filter, $table, $data, $single = FALSE, $required = TRUE) {
    $result = array();
    foreach ($this->getField($filter, $table) as $field => $config) {
      if (isset($data[$field])) {
        switch (isset($config['type']) ? $config['type'] : NULL) {
          case 'int':
            $result[$field] = (int) $data[$field];
            break;

          case 'float':
            $result[$field] = (float) $data[$field];
            break;

          default:
            $result[$field] = $data[$field];
        }
      }
      elseif (!empty($config['not null'])) {
        if (isset($config['default'])) {
          $result[$field] = $config['default'];
        }
        elseif ($required && $config['type'] != 'serial') {
          throw new Exception('Required argument is missing: ' . $field);
        }
      }
    }
    if ($single) {
      return array_shift($result);
    }
    else {
      return $result;
    }
  }

  protected function getFieldName($filter, $table = NULL, $single = FALSE) {
    $result = array_keys($this->getField($filter, $table));
    return $single ? array_shift($result) : $result;
  }

  protected function writeRecord($table, $data, $options = array()) {
    $result = array();
    $exist = isset($options['exist']) ? $options['exist'] : $this->readRecord($table, $data);
    // Update record.
    if ($exist) {
      $query = db_update($this->getDbTable($table));
      foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
        $query->condition($field, $value);
      }
      $result = $query->fields($this->getFieldValue('@not-primary', $table, $data))->execute();
    }
    // Insert record.
    else {
      $result = db_insert($this->getDbTable($table))
          ->fields($this->getFieldValue('@all', $table, $data))
          ->execute();
    }
    return $result;
  }

  protected function readRecord($table, $data, $fields = array()) {
    $result = array();
    if ($primary_fields = $this->getFieldValue('@primary', $table, $data)) {
      $query = db_select($this->getDbTable($table), 't');
      if ($fields) {
        $query->fields('t', $fields);
      }
      else {
        $query->fields('t');
      }
      foreach ($primary_fields as $field => $value) {
        $query->condition("t.$field", $value);
      }
      $result = $query->execute()->fetchAssoc();
    }
    return $result;
  }

  protected function getSelection($table) {
    return new RegisterSelection($this->getDbTable($table));
  }

  protected function fieldGetValue($data) {
    if (is_array($data)) {
      if (array_key_exists(0, $data)) {
        return $data[0];
      }
      else {
        throw new Exception('Invalid arguments');
      }
    }
    else {
      return $data;
    }
  }

  protected function fieldGetOperator($data) {
    if (is_array($data)) {
      if (array_key_exists(1, $data)) {
        return $data[1];
      }
      else {
        throw new Exception('Invalid arguments');
      }
    }
    else {
      return NULL;
    }
  }
}

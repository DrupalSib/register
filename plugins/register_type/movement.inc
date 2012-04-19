<?php

$plugin = array(
  'title' => t('Movement register'),
  'handler' => array(
    'class' => 'RegisterTypeMovement',
    'parent' => 'RegisterType',
  ),
);

class RegisterTypeMovement extends RegisterType {
  protected $periodType;
  protected $remnantsEnabled = FALSE;

  public function init($config) {
    parent::init($config);

    $this->remnantsEnabled = !empty($config['remnants']) ? TRUE : FALSE;
    $this->periodType = isset($config['period']) ? $config['period'] : 'month';

    $this->schema['fields'] = array_merge(
      $this->schema['fields'],
      array(
        'date' => array('type' => 'int', 'size' => 'normal', 'unsigned' => TRUE, 'not null' => TRUE),
        'rtp' => array('type' => 'varchar', 'length' => 64, 'not null' => TRUE),
        'rid' => array('type' => 'int', 'size' => 'normal', 'unsigned' => TRUE, 'not null' => TRUE),
        'rnm' => array('type' => 'int', 'size' => 'normal', 'unsigned' => TRUE, 'not null' => TRUE, 'default' => 0),
        // Remnant selection fields.
        'selection_id' => array('type' => 'varchar', 'length' => 13, 'not null' => TRUE),
        'id' => array('type' => 'serial', 'unsigned' => TRUE, 'not null' => TRUE),
      )
    );

    $this->schema['groups']['registrator']  = array('rtp', 'rid');
    $this->schema['groups']['record']       = array('rtp', 'rid', 'rnm');
    $this->schema['groups']['aggregated']   = $this->getFieldName('#resources');

    // Movement table.
    $this->schema['tables']['movement'] = array(
      'fields' => $this->getField(array('date', '#record', '#dimensions', '#resources', '#details')),
      'indexes' => array(
        'date'        => $this->getFieldName('date'),
        'registrator' => $this->getFieldName('#registrator'),
        'dimension'   => $this->getFieldName('#dimensions'),
      ),
      'unique key' => array(
        'record' => $this->getFieldName(array('date', '#record', '#dimensions')),
      ),
      'primary key' => $this->getFieldName(array('#record')),
    );

    // Content table.
    $this->schema['tables']['content'] = array(
      'fields' => $this->getField(array('#dimensions', '#resources')),
      'primary key' => $this->getFieldName(array('#dimensions')),
    );

    // Turnover table.
    $this->schema['tables']['turnover'] = array(
      'fields' => $this->getField(array('date', '#dimensions', '#resources')),
      'indexes' => array(
        'date' => $this->getFieldName('date'),
      ),
      'primary key' => $this->getFieldName(array('date', '#dimensions')),
    );

    if ($this->remnantsEnabled) {
      // Remnant table.
      $this->schema['tables']['remnant'] = array(
        'fields' => $this->getField(array('date', '#dimensions', '#resources')),
        'indexes' => array(
          'date' => $this->getFieldName('date'),
        ),
        'primary key' => $this->getFieldName(array('date', '#dimensions')),
      );

      // Remnant selection table.
      $this->schema['tables']['remnant_selection'] = array(
        'fields' => $this->getField(array('selection_id', '#dimensions', '#resources')),
        'indexes' => array(
          'selection_id' => $this->getFieldName('selection_id'),
        ),
        'primary key' => $this->getFieldName(array('selection_id', '#dimensions')),
      );

      // Remnant selection aggregation table.
      $indexes = array(
        'selection_id' => $this->getFieldName('selection_id'),
      );
      foreach ($this->getFieldName('#dimensions') as $field) {
        $indexes[$field] = array($field);
      }
      $this->schema['tables']['remnant_selection_agg'] = array(
        'fields' => $this->getField(array('id', 'selection_id', '#dimensions', '#resources')),
        'indexes' => $indexes,
        'primary key' => $this->getFieldName('id'),
      );
    }
  }

  public function getMovements($data) {
    return $this->readRecordsByRegistrator('movement', $data);
  }

  public function getMovementsTotals($filter, $begin = NULL, $end = NULL) {
    $query = db_select($this->getDbTable('movement'), 't');
    $query->fields('t', $this->getFieldName('#dimensions', 'movement'));

    foreach ($this->getFieldName('#resources', 'movement') as $field) {
      $query->addExpression("SUM(t.$field)", $field);
    }

    foreach ($this->getFieldValue('#dimensions', 'movement', $filter, FALSE, FALSE) as $field => $value) {
      $query->condition("t.$field", $value);
    }

    foreach ($this->getFieldName('#dimensions') as $field) {
      $query->groupBy("t.$field");
    }

    if ($begin) {
      $query->condition('t.' . $this->getFieldName('date', 'movement', TRUE), $begin, '>=');
    }

    if ($end) {
      $query->condition('t.' . $this->getFieldName('date', 'movement', TRUE), $end, '<=');
    }

    $result = $query->execute();
    $records = array();

    while ($record = $result->fetchAssoc()) {
      $records[] = $record;
    }

    return $records;
  }

  public function addMovement($data) {
    $this->writeRecord('movement', $data, array('exist' => FALSE));
    $this->writeRecordIncrementaly('content', $data);
    $this->writeRecordTurnover('turnover', $data);
    if ($this->remnantsEnabled) {
      $this->writeRecordRemnant('remnant', $data);
    }
  }

  protected function writeRecordIncrementaly($table, $data, $options = array()) {
    $result = NULL;
    $exist = isset($options['exist']) ? $options['exist'] : $this->readRecord($table, $data);
    // Update record.
    if ($exist) {
      $query = db_update($this->getDbTable($table));
      foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
        $query->condition($field, $value);
      }
      foreach($this->getFieldValue('#aggregated', $table, $data) as $field => $value) {
        $query->expression($field, "$field + :$field", array(":$field" => $value));
      }
      $result = $query->fields($this->getFieldValue('@all', $table, $data))->execute();
    }
    // Insert record.
    else {
      $result = $this->writeRecord($table, $data, array('exist' => FALSE));
    }
    return $result;
  }

  protected function writeRecordTurnover($table, $data) {
    // Set date to the end of the current period.
    $data['date'] = RegisterPeriod::getEndDate($this->periodType, $this->getFieldValue('date', $table, $data, TRUE));
    return $this->writeRecordIncrementaly($table, $data);
  }

  protected function writeRecordRemnant($table, $data) {
    $result = NULL;
    $max_date = RegisterPeriod::getEndDate(
      $this->periodType,
      db_select($this->getDbTable($table), 't')
          ->addExpression('IFNULL(MAX(t.date), 0)', 'date')
          ->execute()
          ->fetchField('date')
    );
    $date = $this->getFieldValue('date', $table, $data, TRUE);

    // Find previous value.
    $query = db_select($this->getDbTable($table), 't');
    foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
      if ($field == 'date') {
        $query->addExpression('IFNULL(MAX(t.date)', 'date', 0);
        $query->condition($field, $value, '<');
      }
      else {
        $query->condition($field, $value);
      }
    }
    $previous_date = $query->execute()->fetchField('date');

    do {
      // Set date to the end of the date's period.
      $data['date'] = RegisterPeriod::getEndDate($this->periodType, $date);
      // Update existing record.
      if ($this->readRecord($table, $data)) {
        $result[] = $this->writeRecordIncrementaly($table, $data, array('exist' => TRUE));
      }
      // Insert new record.
      else {
        // Find previous value.
        $query = db_select($this->getDbTable($table), 't');
        foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
          if ($field == 'date') {
            $query->addExpression('IFNULL(MAX(t.date)', 'date', 0);
            $query->condition($field, $value, '<');
          }
          else {
            $query->condition($field, $value);
          }
        }
        $previous_date = $query->execute()->fetchField('date');

        // Get previous period resources value
        if ($previous_date) {
          $query = db_select($this->getDbTable($table), 't')->fields('t', $this->getFieldName('#aggregated', $table));
          foreach($this->getFieldValue('@primary', $table, $data) as $field => $value) {
            if ($field == 'date') {
              $query->condition($field, $previous_date);
            }
            else {
              $query->condition($field, $value);
            }
          }
          $previous_value = $query->execute()->fetchAssoc();

          // Apply fields.
          foreach($this->getFieldValue('#aggregated', $table, $data) as $field => $value) {
            $query->addExpression("(IFNULL(t.$field, 0) + :$field)", $field, array(":$field" => $value));
          }
          // Fetch and insert modified last date remnants.
          $current_remnant = $query->execute()->fetchAssoc();
          unset($current_remnant['max_date']);
          $data = array_merge($this->getFieldValue('@primary', $table, $data), $current_remnant);
          $result[] = $this->writeRecord($table, $data, array('exist' => FALSE));
        }
        // Just insert new record.
        else {
          $result[] = $this->writeRecord($table, $data, array('exist' => FALSE));
        }
      }
    } while ($date < $max_date);

    return $result;
  }

  public function removeMovements($data) {
    $records = $this->readRecordsByRegistrator('movement', $data);
    $this->removeRecordsByRegistrator('movement', $data);

    if ($records) {
      // Update content.
      foreach($records as $record) {
        $values = $this->getFieldValue('#aggregated', 'content', $record);
        foreach($values as $field => $value) {
          if ($value != 0) {
            $record[$field] = -$value;
          }
        }
        $this->writeRecordIncrementaly('content', $record);
      }

      // Update turnovers.
      foreach($records as $record) {
        $values = $this->getFieldValue('#aggregated', 'turnover', $record);
        foreach($values as $field => $value) {
          if ($value != 0) {
            $record[$field] = -$value;
          }
        }
        $this->writeRecordTurnover('turnover', $record);
      }

      if ($this->remnantsEnabled) {
        // Update remnants.
        foreach ($records as $record) {
          $values = $this->getFieldValue('#aggregated', 'remnant', $record);
          foreach ($values as $field => $value) {
            if ($value != 0) {
              $record[$field] = -$value;
            }
          }
          $this->writeRecordTurnover('remnant', $record);
        }
      }
    }
  }

  protected function removeRecordsByRegistrator($table, $data) {
    $query = db_delete($this->getDbTable($table));
    foreach($this->getFieldValue('#registrator', $table, $data) as $field => $value) {
      $query->condition($field, $value);
    }
    return $query->execute();
  }

  protected function readRecordsByRegistrator($table, $data, $fields = array()) {
    $query = db_select($this->getDbTable($table), 't');
    if ($fields) {
      $query->fields('t', $fields);
    }
    else {
      $query->fields('t');
    }
    foreach($this->getFieldValue('#registrator', $table, $data) as $field => $value) {
      $query->condition("t.$field", $value);
    }

    $result = $query->execute();
    $records = array();
    while ($record = $result->fetchAssoc()) {
      $records[] = $record;
    }
    return $records;
  }

  protected function getMovementsMaxDate() {
    $query = db_select($this->getDbTable('movement'), 't');
    $query->addExpression('IFNULL(MAX(t.date), 0)', 'date');
    return $query->execute()->fetchField();
  }

  public function calculateRemnants($filter) {
    // Create selections table.
    $selection = $this->getSelection('remnant_selection');
    $aggregator = $this->getSelection('remnant_selection_agg');

    // Shroten variables.
    $period_previous_end = RegisterPeriod::getPreviousEndDate($this->periodType, $filter['date']);
    $period_current_begin = RegisterPeriod::getBeginDate($this->periodType, $filter['date']);

    // Fetch movements and remnants.
    $movements = $this->getMovementsTotals($filter, $period_current_begin, $filter['date']);
    $remnants = $this->getRemnants(array_merge($filter, array('date' => $period_previous_end)));

    // Insert data into aggregator table.
    if ($remnants) {
      $aggregator->insert($remnants);
    }
    if ($movements) {
      $aggregator->insert($movements);
    }

    // Aggreagate data and insert into result table.
    $query = $aggregator->select();
    $query->fields('t', $this->getFieldName('#dimensions'));
    foreach ($this->getFieldName('#resources') as $field) {
      $query->addExpression("SUM(t.$field)", $field);
    }
    foreach ($this->getFieldName('#dimensions') as $field) {
      $query->groupBy("t.$field");
    }
    $selection->insert($query);
    $aggregator->clear();
    return $selection;
  }

  public function getRemnants($filter) {
    $records = array();
    if (!isset($filter['date']) || $filter['date'] > $this->getMovementsMaxDate()) {
      $records = $this->getRemnantsCurrent($filter);
    }
    else {
      if ($filter['date'] == RegisterPeriod::getEndDate($this->periodType, $filter['date'])) {
        $records = $this->getRemnantsByPeriod($filter);
      }
      else {
        $records = $this->getRemnantsByDate($filter);
      }
    }
    return $records;
  }

  protected function getRemnantsCurrent($filter) {
    $query = db_select($this->getDbTable('content'), 't');
    $query->fields('t', $this->getFieldName(array('#dimensions', '#resources'), 'content'));
    foreach($this->getFieldValue('#dimensions', 'content', $filter, FALSE, FALSE) as $field => $value) {
      $query->condition("t.$field", $value);
    }
    $result = $query->execute();
    $records = array();
    while ($record = $result->fetchAssoc()) {
      $records[] = $record;
    }
    return $records;
  }

  protected function getRemnantsByPeriod($filter) {
    $records = array();
    $fields = $this->getFieldValue('#dimensions', 'remnant', $filter, FALSE, FALSE);

    // Fetch last remnants date.
    $query = db_select($this->getDbTable('remnant'), 't');
    $query->addExpression('IFNULL(MAX(t.date), 0)', 'date');
    $query->condition('t.date', RegisterPeriod::getEndDate($this->periodType, $filter['date']), '<=');
    foreach($fields as $field => $value) {
      $query->condition("t.$field", $value);
    }
    $date = $query->execute()->fetchField();

    // Fetch last remnants.
    if ($date) {
      $query = db_select($this->getDbTable('remnant'), 't');
      $query->fields('t', $this->getFieldName(array('#dimensions', '#resources'), 'remnant'));
      $query->condition('t.date', $date);
      foreach($fields as $field => $value) {
        $query->condition("t.$field", $value);
      }
      $result = $query->execute();
      while ($record = $result->fetchAssoc()) {
        $records[] = $record;
      }
    }
    return $records;
  }

  protected function getRemnantsByDate($filter) {
    $selection = $this->calculateRemnants($filter);
    $query = $selection->select();
    $query->fields('t', $this->getFieldName(array('#dimensions', '#resources')));
    foreach ($this->getFieldName('#dimensions') as $field) {
      $query->groupBy("t.$field");
    }
    $result = $query->execute();
    $records = array();
    while ($record = $result->fetchAssoc()) {
      $records[] = $record;
    }
    return $records;
  }
}

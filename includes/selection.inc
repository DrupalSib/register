<?php
/**
 * @file
 * Definition of RegisterSelection.
 */

class RegisterSelection {
  protected $id;
  protected $table;

  public function __construct($table, $id = NULL) {
    if (!$table) {
      throw new Exception('Invalid arguments');
    }
    $this->table = $table;
    if ($id) {
      $this->id = $id;
    }
    else {
      $this->id = db_insert('register_selection')
          ->fields(array('updated' => time(), 'clean' => FALSE, 'table' => $table,))
          ->execute();
    }
  }

  /**
   *
   */
  public function clear() {
    db_delete($this->getTable())
        ->condition('selection_id', $this->getId())
        ->execute();
    db_update('register_selection')
        ->fields(array('updated' => time(), 'clean' => TRUE))
        ->condition('id', $this->getId())
        ->execute();
  }

  public function getId() {
    return $this->id;
  }

  public function getTable() {
    return $this->table;
  }

  public function select() {
    $query = db_select($this->table, 't');
    $query->condition('t.selection_id', $this->id);
    return $query;
  }

  public function insert($data) {
    if ($data instanceof SelectQuery) {
      $query = db_insert($this->table);
      $data->addExpression($this->id, 'selection_id');
      $query->from($data);
      return $query->execute();
    }
    elseif (is_array($data)) {
      $query = db_insert($this->table);
      $fields_defined = FALSE;
      foreach ($data as $item) {
        if (!is_array($item)) {
          throw new Exception('Invalid arguments');
        }
        $value = array_merge($item, array('selection_id' => $this->id));
        if (!$fields_defined) {
          $query->fields(array_keys($value));
          $fields_defined = TRUE;
        }
        $query->values($value);
      }

      return $query->execute();
    }
    else {
      throw new Exception('Invalid arguments');
    }
  }
}

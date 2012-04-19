<?php

function register_list() {
  //$reg = register_get('sd');
//  kpr($reg); die;

  ctools_include('export');

  $output = '';

  $header = array(
    t('Name'),
    t('Type'),
    t('Storage'),
    t('Operations'),
  );

  $rows = array();

  $schema = ctools_export_get_schema(REGISTER_TABLE);
  $types = register_types('title');

  foreach (ctools_export_default_list(REGISTER_TABLE, $schema) as $name) {

    $export = ctools_export_crud_load(REGISTER_TABLE, $name);

    $row = array();

    $title = $export->getTitle();
    $row[] = $title ? check_plain($title) : check_plain($export->getName());

    $type = $export->getType();
    $row[] = isset($types[$type]) ? check_plain($types[$type]) : t('Invalid type');

    $storage = $export->getExportTypeTitle();
    $row[] = isset($storage) ? check_plain($storage) : t('Invalid storage');

    $operations = array(
      array(
        'title' => t('Edit'),
        'href' => check_plain("admin/structure/register/{$export->getName()}/edit"),
      ),
    );

    if ($export->getExportType() == EXPORT_IN_DATABASE) {
      $operations[] = array(
        'title' => t('Delete'),
        'href' => check_plain("admin/structure/register/{$export->getName()}/delete"),
      );
    }
    elseif (!$export->isExportInCodeOnly()) {
      $operations[] = array(
        'title' => t('Revert'),
        'href' => check_plain("admin/structure/register/{$export->getName()}/revert"),
      );
    }

    $row[] = theme(
      'links__ctools_dropbutton',
      array('links' => $operations, 'attributes' => array('class' => array('links', 'inline')))
    );

    $rows[] = $row;
  }

  $output .= theme('table', array('header' => $header, 'rows' => $rows));

  return $output;
}

function register_add_form($form, &$form_state) {
  drupal_set_title(t('Add new register'), PASS_THROUGH);

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#required' => TRUE,
  );

  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => register_types('title'),
    '#required' => TRUE,
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function register_add_form_validate($form, &$form_state) {
  ctools_include('export');

  $values = $form_state['values'];
  $schema = ctools_export_get_schema(REGISTER_TABLE);
  $registers = ctools_export_default_list(REGISTER_TABLE, $schema);
  $types = register_types('title');
  $name = $values['name'];
  $type = $values['type'];

  if (isset($registers[$name])) {
    form_set_error('name', t('Register @name exist.', array('@name' => $name)));
  }

  if (!preg_match('!^[a-z0-9_]+$!', $name)) {
    form_set_error('name', t('Register name is invalid.'));
  }

  if (!isset($types[$type])) {
    form_set_error('type', t('Register type is invalid.'));
  }
}

function register_add_form_submit($form, &$form_state) {
  ctools_include('export');

  $values = $form_state['values'];

  $obj = new stdClass();
  $obj->register_name = $values['name'];
  $obj->register_type = $values['type'];

  $export = new RegisterExport($obj);

  ctools_export_crud_save(REGISTER_TABLE, $export);

  $form_state['redirect'] = "admin/structure/register/list";
}

function register_field_delete_form($form, &$form_state, $export, $field) {
  $form['register'] = array(
    '#type' => 'value',
    '#value' => $export->getName(),
  );
  $form['field'] = array(
    '#type' => 'value',
    '#value' => $field,
  );

  $question = t('Delete the @field field?', array('@field' => $field));
  $description = t(
    'This will immediately delete the @field field from the @register register. This cannot be undone.',
    array('@field' => $field, '@register' => $export->getName())
  );

  return confirm_form($form, $question, "admin/structure/register/{$export->getName()}/edit", $description, t('Delete'));
}

function register_field_delete_form_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  $export = ctools_export_crud_load(REGISTER_TABLE, $values['register']);

  $field = $form_state['values']['field'];
  $export->removeField($field);
  ctools_export_crud_save(REGISTER_TABLE, $export);
  drupal_set_message(t('Field @field has been deleted.', array('@field' => $field)));
  $form_state['redirect'] = "admin/structure/register/{$export->getName()}/edit";
}

function register_field_edit_form($form, &$form_state, $register, $field) {
  ctools_include('dependent');

  $form['register'] = array(
    '#type' => 'value',
    '#value' => $register->getName(),
  );
  $form['field'] = array(
    '#type' => 'value',
    '#value' => $field,
  );

  drupal_set_title(
    t('Edit @field field from the @register register', array('@field' => $field, '@register' => $register->getName())),
    PASS_THROUGH
  );

  $field_info = $register->getField($field);

  $form['name'] = array(
    '#type' => 'value',
    '#value' => $field,
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#size' => 32,
    '#maxlength' => 64,
    '#default_value' => isset($field_info['title']) ? $field_info['title'] : '',
  );

  $form['group'] = array(
    '#type' => 'select',
    '#title' => t('Group'),
    '#required' => TRUE,
    '#options' => register_field_groups(),
    '#default_value' => isset($field_info['group']) ? $field_info['group'] : '',
  );

  $form['type_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Type'),
  );

  $form['type_fieldset']['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => register_field_types(),
    '#default_value' => isset($field_info['type']) ? $field_info['type'] : '',
  );

  $form['type_fieldset']['use_size_preset'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use size preset'),
    '#default_value' => isset($field_info['size_preset']) ? TRUE : FALSE,
  );

  $form['type_fieldset']['size'] = array(
    '#type' => 'textfield',
    '#title' => t('Size'),
    '#default_value' => isset($field_info['size']) ? $field_info['size'] : '',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('edit-use-size-preset' => array(0)),
  );

  $form['type_fieldset']['size_preset'] = array(
    '#type' => 'select',
    '#title' => t('Size preset'),
    '#options' => register_field_size_presets(),
    '#default_value' => isset($field_info['size_preset']) ? $field_info['size_preset'] : 'normal',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('edit-use-size-preset' => array(1)),
  );

  $form['options_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options'),
  );

  $form['options_fieldset']['nullable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Nullable'),
    '#default_value' => isset($field_info['nullable']) ? $field_info['nullable'] : FALSE,
  );

  $form['options_fieldset']['default_type'] = array(
    '#type' => 'select',
    '#title' => t('Default type'),
    '#options' => array('none' => t('None'), 'null' => t('NULL'), 'value' => t('Value')),
    '#default_value' => isset($field_info['default_type']) ? $field_info['default_type'] : 'none',
  );

  $form['options_fieldset']['default_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Default value'),
    '#default_value' => isset($field_info['default_value']) ? $field_info['default_value'] : '',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('edit-default-type' => array('value')),
  );

  $form['actions'] = array('#type' => 'actions');

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => "admin/structure/register/{$register->getName()}/edit",
  );

  return $form;
}

function register_field_edit_form_validate($form, &$form_state) {
  $values = $form_state['values'];

  if (empty($values['title'])) {
    form_set_error('title', t('Invalid @field value.', array('@field' => 'title')));
  }

  if (!in_array($values['group'], array_keys(register_field_groups()))) {
    form_set_error('group', t('Invalid @field value.', array('@field' => 'group')));
  }

  if (!in_array($values['type'], array_keys(register_field_types()))) {
    form_set_error('type', t('Invalid @field value.', array('@field' => 'type')));
  }

  if ($values['use_size_preset']) {
    if (!in_array($values['size_preset'], array_keys(register_field_size_presets()))) {
      form_set_error('use_size_preset', t('Invalid @field value.', array('@field' => 'size')));
    }
  }
  else if (!is_numeric($values['size'])) {
    form_set_error('size', t('Invalid @field value.', array('@field' => 'size')));
  }

  if (!in_array($values['default_type'], array_keys(register_field_default_value_types()))) {
    form_set_error('default_type', t('Invalid @field value.', array('@field' => 'default type')));
  }

  if (!$values['nullable'] && ($values['default_type'] == 'null')) {
    form_set_error('default_type', t('Invalid @field value.', array('@field' => 'default type')));
  }
}

function register_field_edit_form_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  $export = ctools_export_crud_load(REGISTER_TABLE, $values['register']);

  $field = $form_state['values']['field'];

  $values = $form_state['values'];

  $info = array();

  $info['title'] = $values['title'];

  $info['group'] = $values['group'];

  $info['type'] = $values['type'];

  if ($values['use_size_preset']) {
    $info['size_preset'] = $values['size_preset'];
  }
  else {
    $info['size'] = $values['size'];
  }

  $info['nullable'] = (bool) $values['nullable'];

  $info['default_type'] = $values['default_type'];

  if ($values['default_type'] == 'null') {
    $info['default_value'] = NULL;
  }
  elseif ($values['default_type'] == 'value') {
    $info['default_value'] = $values['default_value'];
  }

  $export->updateField($field, $info);
  ctools_export_crud_save(REGISTER_TABLE, $export);
  drupal_set_message(t('Field @field has been updated.', array('@field' => $field)));
  $form_state['redirect'] = "admin/structure/register/{$export->getName()}/edit";
}

function register_edit_form($form, &$form_state, $export) {
  drupal_set_title(
    t('Edit @register register', array('@register' => $export->getName())),
    PASS_THROUGH
  );

  $form['register'] = array(
    '#type' => 'value',
    '#value' => $export->getName(),
  );

  $types = array();
  foreach (register_get_plugins('types') as $name => $field) {
    $types[$name] = isset($field['title']) ? $field['title'] : $name;
  }

  $form['fields'] = array(
    '#tree' => TRUE,
  );
  foreach ($export->getFields() as $field => $info) {
    $config = $info;
    $config['operations'] = array(
      array(
        'title' => t('Edit'),
        'href' => "admin/structure/register/{$export->getName()}/field/$field/edit",
      ),
      array(
        'title' => t('Delete'),
        'href' => "admin/structure/register/{$export->getName()}/field/$field/delete",
      ),
    ) ;
    $form['fields'][$field]['#field'] = $config;
    $form['fields'][$field]['#weight'] = isset($info['weight']) ? $info['weight'] : 0;
    $form['fields'][$field]['weight'] = array(
      '#type' => 'textfield',
      '#title' => $info['title'],
      '#title_display' => 'invisible',
      '#size' => 4,
      '#default_value' => isset($info['weight']) ? $info['weight'] : 0,
      '#attributes' => array('class' => array('field-weight')),
    );
  }

  $form['add_field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add new field'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add_field']['new_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#size' => 32,
    '#maxlength' => 64,
  );

  $form['add_field']['new_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#size' => 32,
    '#maxlength' => 64,
  );

  $form['add_field']['new_group'] = array(
    '#type' => 'select',
    '#title' => t('Group'),
    '#options' => register_field_groups(),
  );

  $form['add_field']['new_type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => register_field_types(),
  );

  $form['add_field']['new_add'] = array(
    '#type' => 'submit',
    '#value' => t('Add field'),
    '#validate' => array('register_type_add_field_validate'),
    '#submit' => array('register_type_add_field_submit'),
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function register_delete_form($form, &$form_state, $export) {
  $form['register'] = array(
    '#type' => 'value',
    '#value' => $export->getName(),
  );

  $question = t('Delete the @register register?', array('@register' => $export->getName()));
  $description = t(
    'This will immediately delete @register register. This cannot be undone.',
    array('@register' => $export->getName())
  );

  return confirm_form($form, $question, "admin/structure/register", $description, t('Delete'));
}

function register_delete_form_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  ctools_export_crud_delete(REGISTER_TABLE, $values['register']);
  drupal_set_message(t('Register @register has been deleted.', array('@register' => $values['register'])));
  $form_state['redirect'] = "admin/structure/register";
}

function register_revert_form($form, &$form_state, $export) {
  $form['register'] = array(
    '#type' => 'value',
    '#value' => $export->getName(),
  );

  $question = t('Revert the @register register?', array('@register' => $export->getName()));
  $description = t(
    'This will immediately revert @register register. This cannot be undone.',
    array('@register' => $export->getName())
  );

  return confirm_form($form, $question, "admin/structure/register", $description, t('Revert'));
}

function register_revert_form_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  ctools_export_crud_delete(REGISTER_TABLE, $values['register']);
  drupal_set_message(t('Register @register has been reverted.', array('@register' => $values['register'])));
  $form_state['redirect'] = "admin/structure/register";
}

function register_type_add_field_validate($form, &$form_state) {
  $values = $form_state['values'];

//  // Invalid field title.
//  if (empty($values['new_title'])) {
//    form_set_error('new_title', t('The field !field is invalid.', array('!field' => 'title')));
//  }

  // Invalid field name.
  if (!preg_match('!^[a-z0-9_]+$!', $values['new_name'])) {
    form_set_error('new_name', t('The field !field is invalid.', array('!field' => 'name')));
  }

  // Invalid field group.
  if (in_array($values['new_group'], register_field_groups())) {
    form_set_error('new_group', t('The field !field is invalid.', array('!field' => 'group')));
  }

  // Invalid field type.
  if (in_array($values['new_type'], register_field_types())) {
    form_set_error('new_type', t('The field !field is invalid.', array('!field' => 'type')));
  }
}

function register_type_add_field_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  $export = ctools_export_crud_load(REGISTER_TABLE, $values['register']);

  $field = $values['new_name'];
  $info = array();
  $info['title'] = !empty($values['new_title']) ? $values['new_title'] : $values['new_name'];
  $info['group'] = $values['new_group'];
  $info['type'] = $values['new_type'];
  $info['use_size_preset'] = TRUE;
  $info['size_preset'] = 'normal';

  $export->addField($field, $info);

  ctools_export_crud_save(REGISTER_TABLE, $export);

  drupal_set_message(t('Field @field has been created.', array('@field' => $field)));
}

function register_edit_form_submit($form, &$form_state) {
  ctools_include('export');
  $values = $form_state['values'];
  $export = ctools_export_crud_load(REGISTER_TABLE, $values['register']);

  // Create array for fields reorder function.
  $fields = isset($values['fields']) ? $values['fields'] : array();
  $ordered_fields = array();
  foreach($fields as $field => $info) {
    $ordered_fields[$field] = $info['weight'];
  }
  $export->reorderFields($ordered_fields);

  ctools_export_crud_save(REGISTER_TABLE, $export);
  drupal_set_message(t('Register @register have been updated.', array('@register' => $export->getName())));
}

function register_settings_form($form, &$form_state) {
  $form['update_db'] = array(
    '#type' => 'submit',
    '#value' => t("Update db schema"),
    '#submit' => array('register_settings_submit_update_db_form'),
  );

  return system_settings_form($form);
}

function register_settings_submit_update_db_form($form, &$form_state) {
  foreach (register_get_all() as $register) {
    foreach ($register->getDbSchema() as $table => $info) {
      if (!db_table_exists($table)) {
        db_create_table($table, $info);
        drupal_set_message(t('Table @table updated.', array('@table' => $table)));
      }
      else {
        drupal_set_message(t('Table @table exists.', array('@table' => $table)));
      }
    }
  }
}


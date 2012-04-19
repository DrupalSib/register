<?php

function theme_register_edit_form($variables) {
  $form = $variables['form'];

  $header = array(
    t('Title'),
    t('Group'),
    t('Type'),
    t('Null'),
    t('Default'),
    t('Weight'),
    t('Operations'),
  );

  $groups = register_field_groups();
  $rows = array();
  foreach (element_children($form['fields']) as $field) {
    $config = $form['fields'][$field]['#field'];
    $row = array();

    $row[] = check_plain($config['title']);

    $row[] = check_plain($groups[$config['group']]);

    $type = $config['type'];
    $size = isset($config['size_preset']) ? $config['size_preset'] : $config['size'];
    $row[] = check_plain("$type ($size)");

    $row[] = check_plain(!empty($config['nullable']) ? t('Yes') : '');

    $default_value = '';
    if (!empty($config['default_type'])) {
      if ($config['default_type'] == 'value') {
        $default_value = $config['default_value'];
      }
      elseif ($config['default_type'] == 'null') {
        $default_value = 'NULL';
      }
    }
    $row[] = $default_value;

    $row[] = drupal_render($form['fields'][$field]['weight']);

    $row[] = theme(
      'links__ctools_dropbutton',
      array('links' => $config['operations'], 'attributes' => array('class' => array('links', 'inline')))
    );

    $rows[] = array('data' => $row, 'class' => array('draggable'));
  }

  drupal_add_tabledrag('register-fields', 'order', 'sibling', 'field-weight');

  $output = '';
  $output .= drupal_render($form['name']);
  $output .= drupal_render($form['type']);
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'register-fields')));
  $output .= drupal_render_children($form);

  return $output;
}
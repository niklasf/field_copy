<?php

/**
 * Form constructor for a field copy form.
 *
 * @param $field_instance
 *   A field instance array with information about that field that is going to
 *   be copied.
 *
 * @see field_copy_form_validate()
 * @see field_copy_form_submit()
 * @ingroup forms
 */
function field_copy_form($form, &$form_state, $field_instance) {
  $form_state['field_instance'] = $field_instance;
  $form_state['field'] = field_info_field($field_instance['field_name']);

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Target field label'),
    '#description' => t('The label of the field to copy the values to.'),
    '#required' => TRUE,
  );

  $form['existing_field'] = array(
    '#type' => 'select',
    '#title' => t('Target'),
    '#empty_option' => t('- Create a new field - '),
    '#options' => _field_copy_target_field_options($form_state),
    '#description' => t('The values can only be copied to a field that has the right field type and is not yet on the bundle. Only target fields with same or greater cardinality are available.'),
  );

  $form['field_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Field name'),
    '#description' => t('The name of the new field, that will be created.'),
    '#field_prefix' => 'field_',
    '#size' => 26,
    '#maxlength' => 26,
    '#states' => array(
      'visible' => array(
        ':input[name=existing_field]' => array('value' => ''),
      ),
    ),
  );

  $form['submit'] =  array(
    '#type' => 'submit',
    '#value' => t('Copy field'),
  );

  return $form;
}

/**
 * Creates an associative array of available target fields.
 *
 * @param $form_state
 *   An associative array with 'field_instance' and 'field' set with field API
 *   data structures.
 *
 * @return
 *   An array of fields, keyed by field names.
 */
function _field_copy_target_field_options($form_state) {
  $options = array();

  $field_instance = $form_state['field_instance'];
  $source_field = $form_state['field'];

  foreach (field_info_fields() as $field_name => $field) {
    // Skip fields that have a different type.
    if ($field['type'] != $source_field['type']) {
      continue;
    }

    // Skip fields that are already on the bundle.
    if (!empty($field['bundles'][$field_instance['entity_type']])) {
      $bundles = $field['bundles'][$field_instance['entity_type']];
    }
    else {
      $bundles = array();
    }
    if (in_array($field_instance['bundle'], $bundles)) {
      continue;
    }

    // Skip fields with smaller cardinality.
    if ($source_field['cardinality'] > $field['cardinality'] && $field['cardinality'] != -1) {
      continue;
    }

    $options[$field['field_name']] = t('@field_name (bundles: @bundles)', array('@field_name' => $field['field_name'], '@bundles' => implode(', ', $bundles)));
  }

  return $options;
}

/**
 * Form validation handler for field_copy_form().
 *
 * @see field_copy_form_submit()
 */
function field_copy_form_validate($form, &$form_state) {
  $field_name = 'field_' . $form_state['values']['field_name'];

  if (!preg_match('!^field_[a-z_][a-z0-9_]*$!', $field_name)) {
    form_set_error('field_name', t('The field name %field_name is invalid. The name must include only lowercase unaccentuated letters, numbers and underscores.', array('%field_name' => $field_name)));
  }

  if (strlen($field_name) > 32) {
    form_set_error('field_name', t('The field name can be at most 32 characters long.'));
  }

  if (field_read_fields(array('field_name' => $field_name), array('include_inactive' => TRUE))) {
    form_set_error('field_name', t('The field name %field_name already exists.', array('%field_name' => $field_name)));
  }
}

/**
 * Form submission handler for field_copy_form().
 *
 * @see field_copy_form_validate()
 */
function field_copy_form_submit($form, &$form_state) {
  // Create the new field.
  $new_field = field_create_field(array(
    'field_name' => 'field_' . $form_state['values']['field_name'],
    'type' => $form_state['field']['type'],
    'cardinality' => $form_state['field']['cardinality'],
    'settings' => $form_state['field']['settings'],
  ));

  // Prepare and create the target field instance.
  $new_field_instance = array(
    'label' => $form_state['values']['label'],
    'field_name' => $new_field['field_name'],
  ) + $form_state['field_instance'];
  unset($new_field_instance['id']);
  unset($new_field_instance['field_id']);
  unset($new_field_instance['deleted']);
  $new_field_instance = field_create_instance($new_field_instance);

  // Create a batch process for updating the entities.
  $batch = array(
    'title' => t('Copying field values'),
    'operations' => array(
      array('field_copy_values_batch_step', array($form_state['field_instance'], $new_field_instance))
    ),
    'finished' => 'field_copy_values_batch_finished',
    'file' => drupal_get_path('module', 'field_copy') . '/field_copy.admin.inc',
  );
  batch_set($batch);

  // Redirect to the field overview.
  $entity_info = entity_get_info($new_field_instance['entity_type']);
  $form_state['redirect'] = $entity_info['bundles'][$new_field_instance['bundle']]['admin']['real path'] . '/fields';
}

/**
 * Batch API operation callback for field_copy_form_submit().
 */
function field_copy_values_batch_step($old_instance, $new_instance, &$context) {
  // Prepare an entity query.
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', $new_instance['entity_type'])
    ->entityCondition('bundle', $new_instance['bundle']);

  // Initialize batch process.
  if (empty($context['sandbox'])) {
    $count_query = clone $query;
    $context['sandbox']['max'] = $query->count()->execute();
    $context['sandbox']['current_entity'] = 0;
    $context['sandbox']['progress'] = 0;
    $context['results'] = array(0);
  }

  // Get an array of entities.
  $entities = (array) $query
    ->range(0, 10)
    ->entityCondition('entity_id', $context['sandbox']['current_entity'], '>')
    ->execute();
  $entities = reset($entities);

  // Load field data on the entities.
  field_attach_load($new_instance['entity_type'], $entities);

  foreach ($entities as $id => $entity) {
    // Update each entity.
    $entity->{$new_instance['field_name']} = $entity->{$old_instance['field_name']};
    field_attach_presave($new_instance['entity_type'], $entity);
    field_attach_update($new_instance['entity_type'], $entity);
    dpm($entity);

    $context['sandbox']['current_entity'] = $id;
    $context['sandbox']['progress']++;
    $context['results'][0]++;
  }

  if (count($entities) && $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Batch API finished callback for field_copy_form_submit().
 */
function field_copy_values_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(format_plural($results[0], 'One entity updated.', '@count entities updated.'));
  }
  else {
    drupal_set_message(t('Finished with an error.'), 'error');
  }
}

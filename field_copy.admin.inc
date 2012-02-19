<?php

/**
 * Form constructor for a field copy form.
 *
 * @param $field
 *   A field array with information about that field that is going to be
 *   copied.
 *
 * @see field_copy_form_validate()
 * @see field_copy_form_submit()
 * @ingroup forms
 */
function field_copy_form($form, &$form_state, $field) {
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
    '#options' => array(
      // TODO: Add support for existing fields
    ),
    '#description' => t('The values can only be copied to a field that has the right field type and is not yet on the bundle.'),
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
 * Form validation handler for field_copy_form().
 *
 * @see field_copy_form_submit()
 */
function field_copy_form_validate($form, &$form_state) {
  $field_name = 'field_' . $form_state['values']['field_name'];

  if (!preg_match('!^field_[a-z0-9]+$!', $field_name)) {
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
  drupal_set_message(t('Nothing happens, yet.'));
}

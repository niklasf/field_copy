<?php

/**
 * Form constructor for a field copy form.
 *
 * @param $field
 *   A field array with information about that field that is going to be
 *   copied.
 *
 * @see field_copy_form_submit()
 * @ingroup forms
 */
function field_copy_form($form, &$form_state, $field) {
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Target field label'),
    '#description' => t('The label of the field to copy the values to.'),
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
    '#size' => 20,
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
 * Form submission handler for field_copy_form().
 */
function field_copy_form_submit($form, &$form_state) {
  drupal_set_message(t('Nothing happens, yet.'));
}

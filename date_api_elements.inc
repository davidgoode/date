<?php
// $Id: date_api_elements.inc,v 1.70 2010/08/13 15:46:19 karens Exp $
/**
 * @file
 * Date API elements themes and validation.
 * This file is only included during the edit process to reduce memory usage.
 */

/**
 * Implementation of hook_element_info().
 *
 * Parameters for date form elements, designed to have sane defaults so any
 * or all can be omitted.
 *
 * Fill the element #default_value with a date in datetime format,
 * (YYYY-MM-DD HH:MM:SS), adjusted to the proper local timezone.
 *
 * NOTE - Converting a date stored in the database from UTC to the local zone
 * and converting it back to UTC before storing it is not handled by this
 * element and must be done in pre-form and post-form processing!!
 *
 * The date_select element will create a collection of form elements, with a
 * separate select or textfield for each date part. The whole collection will
 * get re-formatted back into a date value of the requested type during validation.
 *
 * The date_text element will create a textfield that can contain a whole
 * date or any part of a date as text. The user input value will be re-formatted
 * back into a date value of the requested type during validation.
 *
 * The date_timezone element will create a drop-down selector to pick a
 * timezone name.
 *
 * #date_timezone
 *   The local timezone to be used to create this date.
 *
 * #date_format
 *   A format string that describes the format and order of date parts to
 *   display in the edit form for this element. This makes it possible
 *   to show date parts in a custom order, or to leave some of them out.
 *   Be sure to add 'A' or 'a' to get an am/pm selector. Defaults to the
 *   short site default format.
 *
 * #date_label_position
 *   Handling option for date part labels, like 'Year', 'Month', and 'Day',
 *   can be 'above' the date part, 'within' it, or 'none', default is 'above' .
 *   The 'within' option shows the label as the first option in a select list
 *   or the default value for an empty textfield, taking up less screen space.
 *
 * #date_increment
 *   Increment minutes and seconds by this amount, default is 1.
 *
 * #date_year_range
 *   The number of years to go back and forward in a year selector,
 *   default is -3:+3 (3 back and 3 forward).
 *
 * #date_text_parts
 *   Array of date parts that should use textfields instead of selects
 *   i.e. array('year') will format the year as a textfield and other
 *   date parts as drop-down selects.
 */
function _date_api_element_info() {
  $date_base = array(
    '#input' => TRUE, '#tree' => TRUE,
    '#date_timezone' => date_default_timezone(),
    '#date_flexible' => 0,
    '#date_format' => variable_get('date_format_short', 'm/d/Y - H:i'),
    '#date_text_parts' => array(),
    '#date_increment' => 1,
    '#date_year_range' => '-3:+3',
    '#date_label_position' => 'above',
    );
  $type['date_select'] = array_merge($date_base, array(
    '#process' => array('date_select_element_process'),
    '#theme_wrappers' => array('date_select'),
    ));
  $type['date_text'] = array_merge($date_base, array(
    '#process' => array('date_text_element_process'),
    '#theme_wrappers' => array('date_text'),
    ));
  $type['date_timezone'] = array(
    '#input' => TRUE, '#tree' => TRUE,
    '#process' => array('date_timezone_element_process'),
    '#theme_wrappers' => array('date_text'),
    );  
  return $type;
}

/**
 * Create a timezone form element.
 *
 * @param array $element
 * @return array
 *   the timezone form element
 */
function date_timezone_element_process($element, $form_state, $form) {
  $element['#tree'] = TRUE;
  $element['timezone'] = array(
    '#type' => 'select',
    '#title' => theme('date_part_label_timezone', array('part_type' => 'select', 'element' => $element)),
    '#default_value' => $element['#value'],
    '#options' => date_timezone_names($element['#required']),
    '#weight' => $element['#weight'],
    '#required' => $element['#required'],
    '#theme' => 'date_select_element',
    '#theme_wrappers' => array('form_element'),
  );
  if (isset($element['#element_validate'])) {
    array_push($element['#element_validate'], 'date_timezone_validate');
  }
  else {
    $element['#element_validate'] = array('date_timezone_validate');
  }
  // TODO This sometimes causes problems, do we need it?
  //$element['#attributes'] = array('class' => array('date-timezone clear-block'));
  return $element;
}

/**
 * Text date input form.
 *
 * Display all or part of a date in a single textfield.
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from #date_format.
 * 
 * In regular FAPI processing $element['#value'] will contain a string
 * value before the form is submitted, and an array during submission.
 * 
 * In regular FAPI processing $edit is empty until the form is submitted
 * when it will contain an array.
 * 
 */
function date_text_element_process($element, $form_state, $form) {
  $date = NULL;
  
  $granularity = date_format_order($element['#date_format']);
  $edit = !empty($form_state['input']) ? $form_state['values'] : array();
  
  // We sometimes get $edit without $edit['date'] from
  // Views exposed filters.
  if (!empty($edit) && is_array($edit) && !empty($edit['date'])) {
    $date = new DateObject($edit['date'], $element['#date_timezone'], $element['#date_format']);
    if (!$date->valid($granularity, $element['#date_flexible'])) {
      $date = NULL;
    }
  }
  elseif (!empty($element['#value'])) {
    // Value retrieved from database, in ISO format.
    $date = new DateObject($element['#value'], $element['#date_timezone'], DATE_FORMAT_ISO);
    if (!$date->valid($granularity, $element['#date_flexible'])) {
      $date = NULL;
    }
  }
  
  $element['#tree'] = TRUE;
  $element['#theme_wrappers'] = array('date_text');
  
  $element['date']['#type'] = 'textfield';
  $element['date']['#weight'] = !empty($element['date']['#weight']) ? $element['date']['#weight'] : $element['#weight'];
  $element['date']['#default_value'] = is_object($date) ? $date->format($element['#date_format']) : '';
  $element['date']['#attributes'] = array('class' => isset($element['#attributes']['class']) ? $element['#attributes']['class'] += array('date-date') : array('date-date'));
  $element['date']['#description'] = ' ' . t('Format: @date', array('@date' => date_now()->format($element['#date_format'])));
  
  // Keep the system from creating an error message for the sub-element.
  // We'll set our own message on the parent element.
  //$element['date']['#required'] = $element['#required'];
  $element['date']['#theme'] = 'date_textfield_element';
  if (isset($element['#element_validate'])) {
    array_push($element['#element_validate'], 'date_text_validate');
  }
  else {
    $element['#element_validate'] = array('date_text_validate');
  }
  if (!empty($element['#force_value'])) {
    $element['date']['#value'] = $element['date']['#default_value'];
  }

  return $element;
}

/**
 * Flexible date/time drop-down selector.
 *
 * Splits date into a collection of date and time sub-elements, one
 * for each date part. Each sub-element can be either a textfield or a
 * select, based on the value of ['#date_settings']['text_fields'].
 *
 * The exact parts displayed in the field are those in #date_granularity.
 * The display of each part comes from ['#date_settings']['format'].
 * 
 * In regular FAPI processing $element['#value'] will contain a string
 * value before the form is submitted, and an array during submission.
 * 
 * In regular FAPI processing $edit is empty until the form is submitted
 * when it will contain an array.
 * 
 */
function date_select_element_process($element, $form_state, $form) {
  $date = NULL;
  $granularity = date_format_order($element['#date_format']);
  
  if (is_array($element['#value'])) {
    $date = new DateObject($element['#value'], $element['#date_timezone']);
  }
  elseif (!empty($element['#value'])) {
    $date = new DateObject($element['#value'], $element['#date_timezone']);
  }

  $element['#tree'] = TRUE;
  $element['#theme_wrappers'] = array('date_select');
  date_increment_round($date, $element['#date_increment']);
  
  $element += (array) date_parts_element($element, $date, $element['#date_format']);
  
  // Store a hidden value for all date parts not in the current display.
  $granularity = date_format_order($element['#date_format']);
  $formats = array('year' => 'Y', 'month' => 'n', 'day' => 'j', 'hour' => 'H', 'minute' => 'i', 'second' => 's');
  foreach (date_nongranularity($granularity) as $field) {
    if ($field != 'timezone') {
      $element[$field] = array(
        '#type' => 'value',
        '#value' => 0,
        );
    }
  }
  if (isset($element['#element_validate'])) {
    array_push($element['#element_validate'], 'date_select_validate');
  }
  else {
    $element['#element_validate'] = array('date_select_validate');
  }

  return $element;
}

/**
 * Create form elements for one or more date parts.
 *
 * Get the order of date elements from the provided format.
 * If the format order omits any date parts in the granularity, alter the
 * granularity array to match the format, then flip the $order array
 * to get the position for each element. Then iterate through the
 * elements and create a sub-form for each part.
 *
 * @param array $element
 * @param object $date
 * @param array $granularity
 * @param string $format
 * @return array
 *   the form array for the submitted date parts
 */
function date_parts_element($element, $date, $format) {
  $granularity = date_format_order($format);
  $sub_element = array('#granularity' => $granularity);
  $order = array_flip($granularity);

  $hours_format  = strpos(strtolower($element['#date_format']), 'a') ? 'g': 'G';
  $month_function  = strpos($element['#date_format'], 'F') !== FALSE ? 'date_month_names' : 'date_month_names_abbr';
  $count = 0;
  $increment = min(intval($element['#date_increment']), 1);
  foreach ($granularity as $field) {
    // Allow empty value as option if date is not required 
    // or if empty value was provided as a starting point.
    $part_required = ($element['#required'] && is_object($date)) ? TRUE : FALSE;
    $part_type = in_array($field, $element['#date_text_parts']) ? 'textfield' : 'select';
    $sub_element[$field] = array(
      '#weight' => $order[$field],
      '#required' => $element['#required'],
      '#attributes' => array('class' => isset($element['#attributes']['class']) ? $element['#attributes']['class'] += array('date-' . $field) : array('date-' . $field)),
      );
    switch ($field) {
      case 'year':
        $range = date_range_years($element['#date_year_range'], $date);
        $min_year = $range[0];
        $max_year = $range[1];
        
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format('Y') : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = drupal_map_assoc(date_years($min_year, $max_year, $part_required));
        }
        break;
      case 'month':
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format('n') : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = $month_function($part_required);
        }
        break;
      case 'day':
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format('j') : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = drupal_map_assoc(date_days($part_required));
        }
        break;
      case 'hour':
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format($hours_format) : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = drupal_map_assoc(date_hours($hours_format, $part_required));
        }
        $sub_element[$field]['#prefix'] = theme('date_part_hour_prefix', $element);
        break;
      case 'minute':
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format('i') : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = drupal_map_assoc(date_minutes('i', $part_required, $element['#date_increment']));
        }
        $sub_element[$field]['#prefix'] = theme('date_part_minsec_prefix', $element);
        break;
      case 'second':
        $sub_element[$field]['#default_value'] = is_object($date) ? $date->format('s') : '';
        if ($part_type == 'select') {
          $sub_element[$field]['#options'] = drupal_map_assoc(date_seconds('s', $part_required, $element['#date_increment']));
        }
        $sub_element[$field]['#prefix'] = theme('date_part_minsec_prefix', $element);
        break;
    }
    
    // Add handling for the date part label.
    $label = theme('date_part_label_' . $field, array('part_type' => $part_type, 'element' => $element));
    if (in_array($field, $element['#date_text_parts'])) {
      $sub_element[$field]['#type'] = 'textfield';
      $sub_element[$field]['#theme'] = 'date_textfield_element';
      $sub_element[$field]['#size'] = 7;
      if ($element['#date_label_position'] == 'within') {
        if (!empty($sub_element[$field]['#options']) && is_array($sub_element[$field]['#options'])) {
          $sub_element[$field]['#options'] = array(
            '-' . $label => '-' . $label) + $sub_element[$field]['#options'];
        }
        if (empty($sub_element[$field]['#default_value'])) {
          $sub_element[$field]['#default_value'] = '-' . $label;
        }
      }
      elseif ($element['#date_label_position'] != 'none') {
        $sub_element[$field]['#title'] = $label;
      }
    }
    else {
      $sub_element[$field]['#type'] = 'select';
      $sub_element[$field]['#theme'] = 'date_select_element';
      if ($element['#date_label_position'] == 'within') {
        $sub_element[$field]['#options'] = array(
          '' => '-' . $label) + $sub_element[$field]['#options'];
      }
      elseif ($element['#date_label_position'] != 'none') {
        $sub_element[$field]['#title'] = $label;
      }
    }
  }
 
  // Views exposed filters are treated as submitted even if not,
  // so force the #default value in that case. Make sure we set
  // a default that is in the option list.
  if (!empty($element['#force_value'])) {
    $options = $sub_element[$field]['#options'];
    $default = !empty($sub_element[$field]['#default_value']) ? $sub_element[$field]['#default_value'] : array_shift($options);
    $sub_element[$field]['#value'] = $default;
  }
  
  if (($hours_format == 'g' || $hours_format == 'h') && date_has_time($granularity)) {
    $sub_element['ampm'] = array(
      '#type' => 'select',
      '#default_value' => is_object($date) ? (date_format($date, 'G') >= 12 ? 'pm' : 'am') : '',
      '#options' => drupal_map_assoc(date_ampm()),
      '#weight' => 8,
      '#attributes' => array('class' => array('date-ampm')),
    );
    if ($element['#date_label_position'] == 'within') {
      $sub_element['ampm']['#options'] = array('' => '-' . theme('date_part_label_ampm', array('part_type' => 'ampm', 'eleement' => $element))) + $sub_element['ampm']['#options'];
    }
    elseif ($element['#date_label_position'] != 'none') {
      $sub_element['ampm']['#title'] = theme('date_part_label_ampm', array('part_type' => 'ampm', 'element' => $element));
    }
  }
  return $sub_element;
}

/**
 * Helper function to round minutes and seconds to requested value.
 */
function date_increment_round(&$date, $increment) {
  // Round minutes and seconds, if necessary.
  if (is_object($date) && $increment > 1) {
    $day = intval(date_format($date, 'j'));
    $hour = intval(date_format($date, 'H'));
    $second = intval(round(intval(date_format($date, 's')) / $increment) * $increment);
    $minute = intval(date_format($date, 'i'));
    if ($second == 60) {
      $minute += 1;
      $second = 0;
    }
    $minute = intval(round($minute / $increment) * $increment);
    if ($minute == 60) {
      $hour += 1;
      $minute = 0;
    }
    date_time_set($date, $hour, $minute, $second);
    if ($hour == 24) {
      $day += 1;
      $hour = 0;
      $year = date_format($date, 'Y');
      $month = date_format($date, 'n');
      date_date_set($date, $year, $month, $day);
    }
  }
  return $date;
}

/**
 *  Validation function for date selector.
 * 
 * When used as a Views widget, the validation step always gets triggered,
 * even with no form submission. Before form submission $element['#value']
 * contains a string, after submission it contains an array.
 * 
 */
function date_select_validate($element, &$form_state) {
  if (is_string($element['#value'])) {
    return;
  }
  // Strip field labels out of the results.
  foreach ($element['#value'] as $field => $field_value) {
    if (substr($field_value, 0, 1) == '-') {
      $element['#value'][$field] = '';
    }
  }
 
  $error_field = implode('][', $element['#parents']);
  $errors = array();
  $label = !empty($element['#date_title']) ? $element['#date_title'] : (!empty($element['#title']) ? $element['#title'] : '');
  
  if (in_array('year', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['year']))) {
    if ($element['#value']['year'] < variable_get('date_min_year', 1) || $element['#value']['year'] > variable_get('date_max_year', 4000)) {
      $errors[] = t('The year must be a number between %min and %max.', array(
        '%min' => variable_get('date_min_year', 1), '%max' => variable_get('date_max_year', 4000)));  
    }
    else {
      $year = $element['#value']['year'];
    }
  }
  if (in_array('month', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['month']))) {
    if ($element['#value']['month'] < 1 || $element['#value']['month'] > 12) {
      $errors[] = t('The month must be a number between 1 and 12.');
    }
    else {
      $month = $element['#value']['month'];
    }
  }
  if (in_array('day', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['day']))) {
    $min = 1;
    $max = isset($year) && isset($month) ? date_days_in_month($year, $month) : 31;
    if ($element['#value']['day'] < $min || $element['#value']['day'] > $max) {
      $errors[] = t('The day must be a number between !min and !max.', array('!min' => $min, '!max' => $max));
    }
  }
  if (in_array('hour', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['hour']))) {
    $min = isset($element['#value']['ampm']) ? 1 : 0;
    $max = isset($element['#value']['ampm']) ? 12 : 23;
    if ($element['#value']['hour'] < $min || $element['#value']['hour'] > $max) {
      $errors[] = t('The hour must be a number between !min and !max.', array('!min' => $min, '!max' => $max));
    }
  }
  if (in_array('minute', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['minute']))) {
    $min = 0;
    $max = 59;
    if ($element['#value']['minute'] < $min || $element['#value']['minute'] > $max) {
      $errors[] = t('The minute must be a number between !min and !max.', array('!min' => $min, '!max' => $max));
    }
  }
  if (in_array('second', $element['#granularity']) && ($element['#required'] || !empty($element['#value']['second']))) {
    $min = 0;
    $max = 59;
    if ($element['#value']['second'] < $min || $element['#value']['second'] > $max) {
      $errors[] = t('The second must be a number between !min and !max.', array('!min' => $min, '!max' => $max));
    }
  }
  if (isset($element['#value']['ampm'])) {
    if ($element['#value']['ampm'] == 'pm' && $element['#value']['hour'] < 12) {
      $element['#value']['hour'] += 12;
    }
    elseif ($element['#value']['ampm'] == 'am' && $element['#value']['hour'] == 12) {
      $element['#value']['hour'] -= 12;
    }
  }
  $value = date_select_input_value($element);
  if (empty($value) && empty($errors) && $element['#required']) {
    $errors[] = t('A valid value is required.');
  }
  if (!empty($errors)) {
    array_unshift($errors, t('Field %field has errors.', array('%field' => $label)));
    form_set_error($error_field, implode(' ', $errors));
  }
  // If there are no errors and the value is valid, set it.
  if (empty($errors) && !empty($value)) {
    form_set_value($element, $value->format(DATE_FORMAT_ISO), $form_state);
  }
  else {
    form_set_value($element, NULL, $form_state);
  }
}

/**
 * Helper function for extracting a date value out of user input.
 */
function date_select_input_value($element) {
  $granularity = date_format_order($element['#date_format']);
  $value = new DateObject($element['#value']);
  $value->limitGranularity($granularity);
  if ($value->valid($granularity, $element['#date_flexible'])){
    return $value;
  }
  return NULL;
}

/**
 *  Validation for text input.
 * 
 * When used as a Views widget, the validation step always gets triggered,
 * even with no form submission. Before form submission $element['#value']
 * contains a string, after submission it contains an array.
 * 
 */
function date_text_validate($element, &$form_state) {
  if (is_string($element['#value'])) {
    return;
  }
  $parents = $element['#parents'];
  $label = !empty($element['#date_title']) ? $element['#date_title'] : (!empty($element['#title']) ? $element['#title'] : '');
  $value = date_text_input_value($element);
  
  if (empty($value) && !empty($element['#required'])) {
    form_error($element, t('A valid date is required for %title.', array('%title' => $label))); 
  }
  elseif (empty($value) && !empty($element['#value']['date'])) {
    form_error($element, t('%title is invalid.', array('%title' => $label))); 
  }
  elseif (!empty($value)) {
    form_set_value($element, $value->format(DATE_FORMAT_ISO), $form_state);
  }
}

/**
 * Helper function for extracting a date value out of user input.
 */
function date_text_input_value($element) {
  $form_values = $element['#value'];
  $granularity = date_format_order($element['#date_format']);
  $input = $form_values['date'];
  if (!$element['#required'] && trim($input) == '') return NULL;
  $value = new DateObject($input, NULL, $element['#date_format']);
  $value->limitGranularity($granularity);

  if ($value->valid($granularity, $element['#date_flexible'])){
    return $value;
  }
  return NULL;
}

/**
 *  Validation for timezone input
 *
 *  Move the timezone value from the nested field back to the original field.
 */
function date_timezone_validate($element, &$form_state) {
  form_set_value($element, $element['#value']['timezone'], $form_state);
}
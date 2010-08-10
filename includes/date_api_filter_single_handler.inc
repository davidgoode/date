<?php
/*****
Preliminary flexible granularity filter, not yet configured to be used by views.
******/
class date_api_filter_single_handler extends views_handler_filter_numeric {

  /**
   * Custom implementation of query() so we can get the
   * AND and OR methods in the right places.
   */
  function query() {
    $this->ensure_my_table();
    $field = "$this->table_alias.$this->real_field";
        $sql = '';
        $sql_parts = array();
        switch ($this->operator) {
          case 'between':
            $sql_parts[] = $this->date_filter('min', $field, '>=');
            $sql_parts[] = $this->date_filter('max', $field, '<=');
            $sql = implode(' AND ', array_filter($sql_parts));
            break;
          case 'not between':
            $sql_parts[] = $this->date_filter('min', $field, '<');
            $sql_parts[] = $this->date_filter('max', $field, '>');
            $sql = implode(' OR ', array_filter($sql_parts));
            break;
          default:
            $sql = $this->date_filter('value', $field, $this->operator);
            break;
        }
        if (!empty($sql)) {
          // Use set_where_group() with the selected date_method 
          // of 'AND' or 'OR' to combine the field WHERE clauses.
          $this->query->add_where($this->options['group'], $sql);
        }
  }

  function date_filter($prefix, $field, $operator) {
    $field_info = field_info_field($this->definition['content_field_name']);
    $flex = $field_info['settings']['granularity_flexible'];
    $type = $field_info['type'];
    
    
    // Handle the simple operators first.
    if ($operator == 'empty') {
      return $field . ' IS NULL';
    }
    elseif ($operator == 'not empty') {
      return $field . ' IS NOT NULL';
    }

    // Views treats the default values as though they are submitted
    // so we when it is really not submitted we have to adjust the
    // query to match what should have been the default.
    $value_parts = !is_array($this->value[$prefix]) ? array($this->value[$prefix]) : $this->value[$prefix];
    foreach ($value_parts as $part) {
      //$default = $this->default_value($prefix);
      if (!empty($this->force_value) && !empty($default)) {
        $this->value[$prefix] = $default;
      }
      else {
        if (empty($part)) {
          return '';
        }
      }
    }
  
    $granularity = $this->options['granularity'];
    $this->format = date_type_format($type);
    $date = new DateObject($this->value[$prefix]);
    $value = $date->format($this->format);
  
    //for flexible granularity we truncate strings and depend on the usual alphanumeric sort functionality
    switch ($operator) {
      case '>':
      case '>=':
        $sql = $field . " $operator SUBSTRING('$value', LENGTH(". $field ."))";
        break;
      case '<':
      case '<=':
        $length = strlen($value);
        $sql = "SUBSTRING(". $field .", $length) $operator '$value'";
        break;
      case '=':
        $sql = "(".$field." LIKE \"$value%\" OR ".$field." = SUBSTRING('$value', LENGTH(".$field.")))";
        break;
    }
    return $sql;
  }



  // Set default values for the date filter.
  function option_definition() {
    $options = parent::option_definition();
    $options['granularity'] = array('default' => 'day');
    $options['form_type'] = array('default' => 'date_text');
    $options['granularity_flexible'] = 1;
    return $options;
  }

  /**
   * Set the granularity of the date parts to use in the filter.
    */
  function has_extra_options() { return TRUE; }

  /**
   * Date selection options.
   * 
   * TODO Only select widget is working right now.
   */
  function widget_options() {
    $options = array(
      'date_select' => t('Select'), 
      'date_text' => t('Text'), 
      'date_popup' => t('Popup'),
      );
    if (!module_exists('date_popup')) {
      unset($options['date_popup']);
    }  
    return $options;
  }

  function extra_options_form(&$form, &$form_state) {
    $form['form_type'] = array(
      '#type' => 'radios',
      '#title' => t('Date form type'),
      '#default_value' => $this->options['form_type'],
      '#options' => $this->widget_options(),
      '#description' => t('Choose the form element to use for date selection.'),
      );
    $form['granularity_flexible'] = array(
      '#type' => 'checkbox',
      '#title' => t('Flexible Granularity'),
      '#default_value' => $this->options['granularity_flexible'],
      '#description' => t('Allow entry of any date up to the allowed granularity. Most useful for exposed forms.'),
      );
   
    //$form['granularity'] = $this->date_handler->granularity_form($this->options['granularity']);
    //$form['granularity']['#description'] = '<p>' . t("Select a granularity for the date filter. For instance, selecting 'day' will create a filter where users can select the year, month, and day.") . '</p>';
  }
  
  function value_form(&$form, &$form_state) {
    $field_info = field_info_field($this->definition['content_field_name']);
//drupal_set_message("<pre>".print_r($field_info,true)."</pre>");
    parent::value_form(&$form, &$form_state);
    $type = $this->options['form_type'];
    $form['value']['value'] = array_merge($form['value']['value'], array(
      '#type' => $type,
      '#size' => 20,
      '#date_format' => DATE_FORMAT_ISO,
      //just on the input end we can allow flexible date entry or not
      '#date_flexible' => $this->options['granularity_flexible'],
      '#date_label_position' => 'within',
      '#process' => array($type . '_element_process'),
    ));
    $form['value']['min'] = array_merge($form['value']['min'], array(
      '#type' => $type,
      '#size' => 20,
      '#date_format' => DATE_FORMAT_ISO,
      //just on the input end we can allow flexible date entry or not
      '#date_flexible' => $this->options['granularity_flexible'],
      '#date_label_position' => 'within',
      '#process' => array($type . '_element_process'),
    ));
    $form['value']['max'] = array_merge($form['value']['max'], array(
      '#type' => $type,
      '#size' => 20,
      '#date_format' => DATE_FORMAT_ISO,
      //just on the input end we can allow flexible date entry or not
      '#date_flexible' => $this->options['granularity_flexible'],
      '#date_label_position' => 'within',
      '#process' => array($type . '_element_process'),
    ));
  }
  
  
  //function value_validate(&$form, &$form_state) {
  //  drupal_set_message("<pre>".print_r($form_state,true)."</pre>");
  //}
}
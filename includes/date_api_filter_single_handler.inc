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
    $this->get_query_fields();
    if (!empty($this->query_fields)) {
      foreach ((array) $this->query_fields as $query_field) {
        $field = $query_field['field'];
        $sql = '';
        $sql_parts = array();
        switch ($this->operator) {
          case 'between':
            $sql_parts[] = $this->date_filter('min', $query_field, '>=');
            $sql_parts[] = $this->date_filter('max', $query_field, '<=');
            $sql = implode(' AND ', array_filter($sql_parts));
            break;
          case 'not between':
            $sql_parts[] = $this->date_filter('min', $query_field, '<');
            $sql_parts[] = $this->date_filter('max', $query_field, '>');
            $sql = implode(' OR ', array_filter($sql_parts));
            break;
          default:
            $sql = $this->date_filter('value', $query_field, $this->operator);
            break;
        }
        if (!empty($sql)) {
          // Use set_where_group() with the selected date_method 
          // of 'AND' or 'OR' to combine the field WHERE clauses.
          $this->query->set_where_group($this->options['date_method'], 'date');
          $this->query->add_where('date', $sql);
        }
      }
    }
  }

  function date_filter($prefix, $query_field, $operator) {
    $field = $query_field['field'];
    // Handle the simple operators first.
    if ($operator == 'empty') {
      $this->add_date_field($field);
      return $field['fullname'] . ' IS NULL';
    }
    elseif ($operator == 'not empty') {
      $this->add_date_field($field);
      return $field['fullname'] . ' IS NOT NULL';
    }

    // Views treats the default values as though they are submitted
    // so we when it is really not submitted we have to adjust the
    // query to match what should have been the default.
    $value_parts = !is_array($this->value[$prefix]) ? array($this->value[$prefix]) : $this->value[$prefix];
    foreach ($value_parts as $part) {
      $default = $this->default_value($prefix);
      if (!empty($this->force_value) && !empty($default)) {
        $this->value[$prefix] = $default;
      }
      else {
        if (empty($part)) {
          return '';
        }
      }
    }
  
    $this->add_date_field($field);
    $granularity = $this->options['granularity'];
    $date_handler = $query_field['date_handler'];
    $this->format = $date_handler->views_formats($granularity, 'sql');
    $date = new DateObject($this->value[$prefix]);
    $value = $date->format($this->format);
  
    //for flexible granularity we truncate strings and depend on the usual alphanumeric sort functionality
    switch ($operator) {
      case '>':
      case '>=':
        $sql = $field['fullname'] . " $operator SUBSTRING('$value', LENGTH(". $field['fullname'] ."))";
        break;
      case '<':
      case '<=':
        $length = strlen($value);
        $sql = "SUBSTRING(". $field['fullname'] .", $length) $operator '$value'";
        break;
      case '=':
        $sql = "(".$field['fullname']." LIKE '$value%' OR ".$field['fullname']." = SUBSTRING('$value', LENGTH(".$field['fullname'].")))";
        break;
    }
    return $sql;
  }

}
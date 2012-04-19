<?php

/**
 * @file
 * Definition of RegisterPeriod.
 */

class RegisterPeriod {
  const DAY = 'day';
  const WEEK = 'week';
  const MONTH = 'month';

  static function getBeginDate($type, $timestamp) {
    $date = new DateTime();
    $date->setTimestamp($timestamp);
    switch($type) {
      case self::DAY:
        $date->modify('00:00:00');
        break;

      case self::WEEK:
        $date->modify('first day of this week 00:00:00');
        break;

      case self::MONTH:
        $date->modify('first day of this month 00:00:00');
        break;

      default:
        throw new Exception('Unknown period type: ' . $type);
    }
    return $date->getTimestamp();
  }

  static function getEndDate($type, $timestamp) {
    $date = new DateTime();
    $date->setTimestamp($timestamp);
    switch($type) {
      case self::DAY:
        $date->modify('23:59:59');
        break;

      case self::WEEK:
        $date->modify('last day of this week 23:59:59');
        break;

      case self::MONTH:
        $date->modify('last day of this month 23:59:59');
        break;

      default:
        throw new Exception('Unknown period type: ' . $type);
    }
    return $date->getTimestamp();
  }

  static function getNextEndDate($type, $timestamp) {
    $date = new DateTime();
    $date->setTimestamp($timestamp);
    switch($type) {
      case self::DAY:
        $date->modify('next day 23:59:59');
        break;

      case self::WEEK:
        $date->modify('last day of next week 23:59:59');
        break;

      case self::MONTH:
        $date->modify('last day of next month 23:59:59');
        break;

      default:
        throw new Exception('Unknown period type: ' . $type);
    }
    return $date->getTimestamp();
  }

  static function getPreviousEndDate($type, $timestamp) {
    $date = new DateTime();
    $date->setTimestamp($timestamp);
    switch($type) {
      case self::DAY:
        $date->modify('previous day 23:59:59');
        break;

      case self::WEEK:
        $date->modify('last day of previous week 23:59:59');
        break;

      case self::MONTH:
        $date->modify('last day of previous month 23:59:59');
        break;

      default:
        throw new Exception('Unknown period type: ' . $type);
    }
    return $date->getTimestamp();
  }
}

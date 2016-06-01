<?php
// Пройти по всем директориям модулей

// Для каждого с незаданным инфо - вытащить инфу о версии
// drushpm injectinfofile
function upvi_drush_command() {
  $items = array();

  $items['update-version-info'] = array(
    'description' => "Injects version to info file of ALL contrib modules",
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

function drush_upvi_update_version_info() {
  $options = array(
    'type' => 'module',
    'no-core' => TRUE,
    'pipe' => TRUE);

  $modules_list = drush_invoke_process('@self', 'pm-list', array(), $options);
  foreach ($modules_list['object'] as $project_name => $data) {
    $project_dir = drupal_get_path('module', $project_name);
    if(empty($data['version'])) {
      $version = _upvi_compute_version($project_dir);

      if (drush_shell_cd_and_exec($project_dir, 'git log -1 --pretty=format:%ct')) {
        $output = drush_shell_exec_output();
        $datestamp = $output[0];
      }

      if(_upvi_inject_info_file_metadata($project_dir, $project_name, $version, $datestamp)) {
        drush_print('Injected version info to ' . $project_name);
      }
    }
  }
}


function _upvi_compute_version($project_dir) {
  $rebuild_version = '';

  if (drush_shell_cd_and_exec($project_dir, 'git describe --tags')) {
    $shell_output = drush_shell_exec_output();
    $rebuild_version = $shell_output[0];
  }

  return $rebuild_version;
}

function _upvi_inject_info_file_metadata($project_dir, $project_name, $version, $datestamp) {
  $info_files = drush_scan_directory($project_dir, '/.*' . '.info' . '$/');
  if (!empty($info_files)) {
    $info = _upvi_generate_info_ini_metadata($version, $project_name, $datestamp);
    foreach ($info_files as $info_file) {
      if (!drush_file_append_data($info_file->filename, $info)) {
        return FALSE;
      }
    }
  }
  return TRUE;
}

function _upvi_generate_info_ini_metadata($version, $project_name, $datestamp) {
  $matches = array();
  $extra = '';
  if (preg_match('/^((\d+)\.x)-.*/', $version, $matches) && $matches[2] >= 6) {
    $extra .= "\ncore = \"$matches[1]\"";
  }
  $date = date('Y-m-d', $datestamp);
  $info = <<<METADATA

; Information added by drush on {$date}
project = "{$project_name}"
version = "{$version}"
datestamp = "{$datestamp}"
METADATA;
  return $info;
}

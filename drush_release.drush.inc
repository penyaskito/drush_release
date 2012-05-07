<?php

/**
 * Implementation of hook_drush_command().
 */
function drush_release_drush_command() {
  $items['release-patch'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Creates a release patch package from two git tags.',
    'arguments' => array(
      'tag0' => 'Tag of the last released codebase.',
      'tag1' => 'Tag of the new release codebase.',
      'path' => 'The path at which to make the package.',
    ),
    'examples' => array(
      'drush release-patch production-20100201 production-20100203 ./example' => 'Build the release patch in the example directory.',
    ),
    'options' => array(
      'tar' => 'Create the result as a tar file.',
      'no-trunk' => 'Avoid last directory before drush directory',
    ),
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function drush_release_drush_help($section) {
  switch ($section) {
  case 'drush:release-patch':
    return dt('Creates a release patch package from two git tags.');
  }
}

/**
 * Drush callback; release-patch based on two git tags.
 */
function drush_drush_release_release_patch($tag0, $tag1, $path = NULL) {
  // By default, git pages with less. The pipe to cat avoids it without touching git global core.pager property.
  drush_shell_exec('git diff -F --name-status %s %s -- | cat', $tag0, $tag1);
  $diffed_files = drush_shell_exec_output();

  $added = array();
  $deleted = array();
  $modified = array();
  foreach ($diffed_files as $diff_file) {
    $diff = explode("\t", $diff_file);
    switch ($diff[0]){
    case 'A':
      $added[] = $diff[1];
      break;
    case 'D':
      $deleted[] = $diff[1];
      break;
    case 'M':
      $modified[] = $diff[1];
      break;
    }
    drush_log(dt("File %path has been %status.", array('%path' => $diff[1], '%status' => $diff[0] == 'A' ? 'added' : 'modified')), 'debug');
  }
  
  $drupal_path = drush_locate_root();
  $patch_path = drush_cwd() .'/'. $path;
  $files_path = explode(DIRECTORY_SEPARATOR, $drupal_path);
  if (drush_get_option('no-trunk', FALSE)) {
    $files_path = array_splice($files_path, 0, count($files_path) - 1);
  }
  $files_path = implode(DIRECTORY_SEPARATOR, $files_path) . DIRECTORY_SEPARATOR;
  drush_mkdir($patch_path);
  drush_log(dt("%folder created", array('%folder' => $patch_path)), 'debug'); 
  chdir($drupal_path);

  foreach ($added as $afile) {
    $folder = $patch_path .'/'. $afile;
    drush_mkdir(dirname($folder));
    drush_log(dt("%folder created", array('%folder' => dirname($folder))),'debug'); 
    copy($files_path . $afile, $folder);
    drush_log(dt("%file copied to %dir", array('%file'=> $afile, '%dir'=>$folder)), 'debug');
  }  
  foreach ($modified as $mfile) {
    $folder = $patch_path .'/'. $mfile;
    drush_mkdir(dirname($folder));
    drush_log(dt("%folder created", array('%folder' => $folder)),'debug'); 
    copy($files_path . $mfile, $folder);
    drush_log(dt("%file copied to %dir", array('%file'=> $mfile, '%dir'=>$folder)), 'debug');
  }
  if (!empty($deleted)) {
    $bash_header = "#!/bin/sh\n";
    $rmfile .= $bash_header;
    $rmdirs = array();
    foreach ($deleted as $dfile) {
      drush_log(dt("%file removed", array('%file' => $dfile)), 'debug');
      $rmfile .= "rm -rf $dfile\n";
      drush_log(dirname($dfile) ." folder being deleted", 'debug');
      if (!file_exists(dirname($dfile))) {
        if (! in_array(dirname($dfile), $rmdirs)) {
          $rmdirs[] = dirname($dfile);
          drush_log(dt("%folder will be deleted.", array('%folder' => dirname($dfile))), 'debug');
        }
      }
    }
    foreach ($rmdirs as $folder) {
     $rmfile .= "rm -rf $folder \n";
    }
    if (!file_exists("$patch_path/scripts")) {
      drush_mkdir("$patch_path/scripts");
    }
    $fh = fopen("$patch_path/scripts/$path.sh", 'w');
    fwrite($fh, $rmfile);
    fclose($fh);
  }
  
  if (drush_get_option('tar', FALSE)) {
    if (empty($path)) {
      $patch_path .= date('YmdHis');
    }
    drush_shell_cd_and_exec(dirname($patch_path), "tar -cf %s %s", $patch_path .'.tar', basename($patch_path));
  }
}

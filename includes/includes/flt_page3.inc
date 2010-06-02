<?php
  // floten3.php
  // @version 1.0
  // @copyright 2008 by Chlorel for XNova

  // Test de coherance de la destination (voir si elle se trouve dans les limites de l'univers connu
  $errorlist = "";
  if (!$galaxy || $galaxy > MAX_GALAXY_IN_WORLD || $galaxy < 1)
    $errorlist .= $lang['fl_limit_galaxy'];
  if (!$system || $system > MAX_SYSTEM_IN_GALAXY || $system < 1)
    $errorlist .= $lang['fl_limit_system'];
  if (!$planet || $planet < 1 || ($planet > MAX_PLANET_IN_SYSTEM AND $target_mission != MT_EXPLORE ))
    $errorlist .= $lang['fl_limit_planet'];
  if ($planetrow['galaxy'] == $galaxy && $planetrow['system'] == $system && $planetrow['planet'] == $planet && $planetrow['planet_type'] == $planet_type)
    $errorlist .= $lang['fl_ownpl_err'];
  if (!$planet_type)
    $errorlist .= $lang['fl_no_planettype'];
  if ($planet_type != PT_PLANET AND $planet_type != PT_DEBRIS AND $planet_type != PT_MOON)
    $errorlist .= $lang['fl_fleet_err_pl'];
  if (empty($missiontype[$target_mission])) {
    $errorlist .= $lang['fl_bad_mission'];
  }

  $TransMetal      = max(0, intval($_POST['resource1']));
  $TransCrystal    = max(0, intval($_POST['resource2']));
  $TransDeuterium  = max(0, intval($_POST['resource3']));
  $StorageNeeded   = $TransMetal + $TransCrystal + $TransDeuterium;

  if ($StorageNeeded < 1 AND $target_mission == MT_TRANSPORT) {
    $errorlist .= $lang['fl_noenoughtgoods'];
  }

  if ($target_mission == MT_EXPLORE) {
    if ($MaxExpeditions == 0 ) {
      $errorlist .= $lang['fl_expe_notech'];
    } elseif ($FlyingExpeditions >= $MaxExpeditions ) {
      $errorlist .= $lang['fl_expe_max'];
    }
  } else {
    if ($TargetPlanet['id_owner']){
      if ($target_mission == MT_COLONIZE)
        $errorlist .= $lang['fl_colonized'];

      if ($TargetPlanet["id_owner"] == $planetrow["id_owner"]){
        if ($target_mission == MT_ATTACK)
          $errorlist .= $lang['fl_no_self_attack'];

        if ($target_mission == MT_SPY)
          $errorlist .= $lang['fl_no_self_spy'];
      }else{
        if ($target_mission == MT_RELOCATE)
          $errorlist .= $lang['fl_only_stay_at_home'];
      }
    }else{
      if ($target_mission < MT_COLONIZE){
        $errorlist .= $lang['fl_unknow_target'];
      }else{
        if ($target_mission == MT_DESTROY)
          $errorlist .= $lang['fl_nomoon'];

        if ($target_mission == MT_RECYCLE){
          $debris = doquery("SELECT * FROM {{table}} WHERE galaxy = {$galaxy} and system = {$system} AND planet = {$planet}", "galaxy", true);
          if($debris['metal']+$debris['crystal']==0)
            $errorlist .= $lang['fl_nodebris'];
        }
      }
    }
  }


  $fleetarray  = unserialize(base64_decode(str_rot13($_POST["usedfleet"])));
  if (!is_array($fleetarray))
    $errorlist .= $lang['fl_no_fleetarray'];

  if ($errorlist)
    message ("<font color=\"red\"><ul>" . $errorlist . "</ul></font>", $lang['fl_error'], "fleet." . $phpEx, 2);

//  if (!isset($fleetarray)) {
//    message ("<font color=\"red\"><b>". $lang['fl_no_fleetarray'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
//  }
  // On verifie s'il y a assez de vaisseaux sur la planete !
  foreach ($fleetarray as $Ship => $Count) {
    if ($Count > $planetrow[$resource[$Ship]]) {
      message ("<font color=\"red\"><b>". $lang['fl_fleet_err'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
    }
  }


  //Normally... unless its acs...
  $fleet_group = max(0, intval($_POST['fleet_group']));
  //But is it acs??
  //Well all acs fleets must have a fleet code.
  if($fleet_group){
    //Also it must be mission type 2
    if($target_mission == MT_AKS){
      //The co-ords must be the same as where the acs fleet is going.
      $target = "g".$galaxy."s".$system."p".$planet."t".$planet_type;
      if($_POST['acs_target_mr'] == $target){
        //ACS attack must exist (if acs fleet has arrived this will also return false (2 checks in 1!!!)
        $aks = doquery("SELECT * FROM {{table}} WHERE id = {$fleet_group}",'aks', true);
        if (!$aks){
          $fleet_group = 0;
        }else{
          $galaxy = $aks['galaxy'];
          $system = $aks['system'];
          $planet = $aks['planet'];
          $planet_type = $aks['planet_type'];
        }
      }
    }
  }
  //Check that a failed acs attack isn't being sent, if it is, make it an attack fleet.
  if((!$fleet_group) && ($target_mission == MT_AKS)){
    $target_mission = MT_ATTACK;
  }

  CheckPlanetUsedFields($planetrow);

  $protection      = $game_config['noobprotection'];
  $protectiontime  = $game_config['noobprotectiontime'];
  $protectionmulti = $game_config['noobprotectionmulti'];
  if ($protectiontime < 1) {
    $protectiontime = 9999999999999999;
  }

  $MyDBRec = doquery("SELECT * FROM {{table}} WHERE `id` = {$user['id']};", 'users', true);
  if ($TargetPlanet['id_owner'] == '') {
    $HeDBRec = $MyDBRec;
  } elseif ($TargetPlanet['id_owner'] != '') {
    $HeDBRec = doquery("SELECT * FROM {{table}} WHERE `id` = '". $TargetPlanet['id_owner'] ."';", 'users', true);
  }
  if ($HeDBRec['onlinetime'] < ($time_now - 60*60*24*7)) {
    $protectiontime = 0;
  }
  if ($MyDBRec['user_lastip'] == $HeDBRec['user_lastip'] AND $MyDBRec['user_lastip'] > 0 AND $HeDBRec['id'] != $MyDBRec['id']) {
    message ("<font color=\"red\"><b>". $lang['fl_multi_ip_protection'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }

  $MyGameLevel = doquery("SELECT total_points FROM {{table}} WHERE `stat_type` = '1' AND `stat_code` = '1' AND `id_owner` = '". $MyDBRec['id'] ."';", 'statpoints', true);
  $HeGameLevel = doquery("SELECT total_points FROM {{table}} WHERE `stat_type` = '1' AND `stat_code` = '1' AND `id_owner` = '". $HeDBRec['id'] ."';", 'statpoints', true);
  $MyGameLevel = $MyGameLevel['total_points'];
  $HeGameLevel = $HeGameLevel['total_points'];

  switch ($target_mission){
    case MT_ATTACK:
    case MT_AKS:
    case MT_HOLD:
    case MT_SPY:
    case MT_DESTROY:
      if ($TargetPlanet['id_level'] > $user['authlevel'])
        message ("<font color=\"red\"><b>". $lang['fl_adm_attak'] ."</b></font>", $lang['fl_warning'], "fleet." . $phpEx, 2);

      if( ($TargetPlanet['id_owner'] AND $protection) AND (
        (($HeGameLevel * $protectionmulti) < $MyGameLevel AND $HeGameLevel < ($protectiontime * 1000))
        OR
        (($MyGameLevel * $protectionmulti) < $HeGameLevel AND $MyGameLevel < ($protectiontime * 1000))
      ) ) message("<font color=\"lime\"><b>".$lang['fl_noob_mess_n']."</b></font>", $lang['fl_noob_title'], "fleet." . $phpEx, 2);
      break;
    default:
  }

  if ($HeDBRec['urlaubs_modus'] AND $target_mission != MT_RECYCLE) {
    message("<font color=\"lime\"><b>".$lang['fl_vacation_pla']."</b></font>", $lang['fl_vacation_ttl'], "fleet." . $phpEx, 2);
  }

  if ($target_mission != MT_EXPLORE) {
    if ($HeDBRec['ally_id'] != $MyDBRec['ally_id'] AND $target_mission == MT_RELOCATE) {
      message ("<font color=\"red\"><b>". $lang['fl_only_stay_at_home'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
    }
    if ($TargetPlanet['ally_deposit'] < 1 AND $HeDBRec != $MyDBRec AND $target_mission == MT_HOLD) {
      message ("<font color=\"red\"><b>". $lang['fl_no_allydeposit'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
    }
  }

  $speed_possible = array(10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
  $AllFleetSpeed  = GetFleetMaxSpeed ($fleetarray, 0, $user);
  $GenFleetSpeed  = intval($_POST['speed']);
  $MaxFleetSpeed  = min($AllFleetSpeed);
  if (!in_array($GenFleetSpeed, $speed_possible)) {
    message ("<font color=\"red\"><b>". $lang['fl_cheat_speed'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }
  if ($MaxFleetSpeed != $_POST['speedallsmin']) {
    message ("<font color=\"red\"><b>". $lang['fl_cheat_speed'] ."123</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }

  $distance      = GetTargetDistance ( $planetrow['galaxy'], $galaxy, $planetrow['system'], $system, $planetrow['planet'], $planet );
  $duration      = GetMissionDuration ( $GenFleetSpeed, $MaxFleetSpeed, $distance, $SpeedFactor );
  $consumption   = GetFleetConsumption ( $fleetarray, $SpeedFactor, $duration, $distance, $MaxFleetSpeed, $user );

  $fleet['start_time'] = $duration + $time_now;
  if ($target_mission == MT_EXPLORE OR $target_mission == MT_HOLD) {
    $StayDuration = max(0,intval($_POST['missiontime'])) * 3600;
    $StayTime     = $fleet['start_time'] + $StayDuration;
  } else {
    $StayDuration = 0;
    $StayTime     = 0;
  }
  $fleet['end_time']   = $StayDuration + (2 * $duration) + $time_now;

  if ($aks AND $target_mission==MT_AKS){
    if ($fleet['start_time']>$aks['ankunft'])
      message ($lang['fl_aks_too_slow'] . 'Fleet arrival: ' . date(DATE_TIME,$fleet['start_time']) . " AKS arrival: " .date(DATE_TIME,$aks['ankunft']), $lang['fl_error']);
    $fleet['start_time'] = $aks['ankunft'];
    $fleet['end_time'] = $aks['ankunft'] + $duration;
  };

  $FleetStorage        = 0;
  $FleetShipCount      = 0;
  $fleet_array         = "";
  $FleetSubQRY         = "";
  foreach ($fleetarray as $Ship => $Count) {
    $FleetStorage    += $pricelist[$Ship]["capacity"] * $Count;
    $FleetShipCount  += $Count;
    $fleet_array     .= $Ship .",". $Count .";";
    $FleetSubQRY     .= "`{$resource[$Ship]}` = `{$resource[$Ship]}` - {$Count} , ";
  }
  $FleetStorage        -= $consumption;

  if ( $StorageNeeded > $FleetStorage) {
    message ("<font color=\"red\"><b>". $lang['fl_nostoragespa'] . pretty_number($StorageNeeded - $FleetStorage) ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }
  if ($planetrow['deuterium'] < $TransDeuterium + $consumption) {
    message ("<font color=\"red\"><b>". $lang['fl_no_deuterium'] . pretty_number($TransDeuterium + $consumption - $planetrow['deuterium']) ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }
  if (($planetrow['metal'] < $TransMetal) || ($planetrow['crystal'] < $TransCrystal)) {
    message ("<font color=\"red\"><b>". $lang['fl_no_resources'] ."</b></font>", $lang['fl_error'], "fleet." . $phpEx, 2);
  }

  // ecriture de l'enregistrement de flotte (a partir de l��_, y a quelque chose qui vole et c'est toujours sur la planete d'origine)
  $QryInsertFleet  = "INSERT INTO {{table}} SET ";
  $QryInsertFleet .= "`fleet_owner` = '". $user['id'] ."', ";
  $QryInsertFleet .= "`fleet_mission` = '". $target_mission ."', ";
  $QryInsertFleet .= "`fleet_amount` = '". $FleetShipCount ."', ";
  $QryInsertFleet .= "`fleet_array` = '". $fleet_array ."', ";
  $QryInsertFleet .= "`fleet_start_time` = '". $fleet['start_time'] ."', ";
  $QryInsertFleet .= "`fleet_start_galaxy` = '". intval($planetrow['galaxy']) ."', ";
  $QryInsertFleet .= "`fleet_start_system` = '". intval($planetrow['system']) ."', ";
  $QryInsertFleet .= "`fleet_start_planet` = '". intval($planetrow['planet']) ."', ";
  $QryInsertFleet .= "`fleet_start_type` = '". intval($planetrow['planet_type']) ."', ";
  $QryInsertFleet .= "`fleet_end_time` = '". $fleet['end_time'] ."', ";
  $QryInsertFleet .= "`fleet_end_stay` = '". $StayTime ."', ";
  $QryInsertFleet .= "`fleet_end_galaxy` = '". $galaxy ."', ";
  $QryInsertFleet .= "`fleet_end_system` = '". $system ."', ";
  $QryInsertFleet .= "`fleet_end_planet` = '". $planet ."', ";
  $QryInsertFleet .= "`fleet_end_type` = '". $planet_type ."', ";
  $QryInsertFleet .= "`fleet_resource_metal` = '". $TransMetal ."', ";
  $QryInsertFleet .= "`fleet_resource_crystal` = '". $TransCrystal ."', ";
  $QryInsertFleet .= "`fleet_resource_deuterium` = '". $TransDeuterium ."', ";
  $QryInsertFleet .= "`fleet_target_owner` = '". $TargetPlanet['id_owner'] ."', ";
  $QryInsertFleet .= "`fleet_group` = '". $fleet_group ."', ";
  $QryInsertFleet .= "`start_time` = '". $time_now ."';";
  doquery( $QryInsertFleet, 'fleets');

  $planetrow["metal"]     -= $TransMetal;
  $planetrow["crystal"]   -= $TransCrystal;
  $planetrow["deuterium"] -= $TransDeuterium;
  $planetrow["deuterium"] -= $consumption;

  $QryUpdatePlanet  = "UPDATE {{table}} SET ";
  $QryUpdatePlanet .= $FleetSubQRY;
  // $QryUpdatePlanet .= "`metal` = '". $planetrow["metal"] ."', ";
  // $QryUpdatePlanet .= "`crystal` = '". $planetrow["crystal"] ."', ";
  // $QryUpdatePlanet .= "`deuterium` = '". $planetrow["deuterium"] ."' ";
  $QryUpdatePlanet .= "`metal` = `metal` - {$TransMetal}, ";
  $QryUpdatePlanet .= "`crystal` = `crystal` - {$TransCrystal}, ";
  $QryUpdatePlanet .= "`deuterium` = `deuterium` - {$TransDeuterium} - {$consumption} ";
  $QryUpdatePlanet .= "WHERE ";
  $QryUpdatePlanet .= "`id` = '". $planetrow['id'] ."'";

  // Mise a jours de l'enregistrement de la planete de depart (a partir de l?�, y a quelque chose qui vole et ce n'est plus sur la planete de depart)
  doquery("LOCK TABLE {{table}} WRITE", 'planets');
  doquery ($QryUpdatePlanet, "planets");
  doquery("UNLOCK TABLES", '');

//  if ($fleet_group) {
//    // doquery("UPDATE `{{table}}` SET `p_num` = `p_num` + 1 WHERE `id` = '{$fleet_group}';", 'aks');
//  };

  $parse["mission"] = $lang['type_mission'][$target_mission];
  if ($target_mission == MT_EXPLORE OR $target_mission == MT_HOLD) {
    // $parse["mission"] .= sprintf($lang['fl_duration_time'], $StayDuration);
    $parse["mission"] .= ' ' . pretty_time($StayDuration);
  };
  $parse["dist"] = pretty_number($distance);
  $parse["speed"] = pretty_number($MaxFleetSpeed);
  $parse["deute_need"] = pretty_number($consumption);
  $parse["from"] = $planetrow['galaxy'].":".$planetrow['system'].":".$planetrow['planet'];
  $parse["time_go"] = date(DATE_TIME, $fleet['start_time']);
  $parse["time_back"] = date(DATE_TIME, $fleet['end_time']);

  $parse_temp['DisplayControls'] = 'display: none;';
  $$ShipList = "";
  foreach ($fleetarray as $Ship => $Count) {
    $parse_temp['ShipSpeed'] =
    $parse_temp['ShipName'] = $lang['tech'][$Ship];
    $parse_temp['ShipNumPrint'] = pretty_number($Count);
    $ShipList .= parsetemplate(gettemplate('fleet_ship_row'), $parse_temp);
  }
  $parse['ShipList'] = $ShipList;

  // Provisoire
  sleep (1);

  $planetrow = doquery ("SELECT * FROM {{table}} WHERE `id` = '". $planetrow['id'] ."';", 'planets', true);

  $page = parsetemplate(gettemplate('fleet3'), $parse);
  display($page, $lang['fl_title']);
?>
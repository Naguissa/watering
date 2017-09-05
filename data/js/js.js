(function () {
	"use strict";

	function resetSD() {
		$.ajax({
			url: '/api/resetSD',
			method: 'GET',
			dataType: 'json'
		}).done(function (data) {
			alert("SD card reset done");
		});
	}
	
	
	function getRTC() {
		$.ajax({
			url: '/api/status',
			method: 'GET',
			dataType: 'json'
		}).done(function (data) {
			var dtparts = data.date.split(' '), date = dtparts[0].split('-'), time = dtparts[1].split(':')
			var content = '<h2>Check and set RTC</h2>';
			content += '<p>Advice: maintain RTC on GMT without DST.</p>';
			content += '<div class="rtc">';
			content += '<div><label for="year">Year</label><input type="text" name="year" id="app-year" value="' + date[0] + '"></div>';
			content += '<div><label for="year">Month</label><input type="text" name="month" id="app-month" value="' + date[1] + '"></div>';
			content += '<div><label for="year">Day</label><input type="text" name="day" id="app-day" value="' + date[2] + '"></div>';
			content += '<div><label for="year">Day of week</label><input type="text" name="dow" id="app-dow" value="' + data.dow + '"></div>';
			content += '<div><label for="year">Hour</label><input type="text" name="hour" id="app-hour" value="' + time[0] + '"></div>';
			content += '<div><label for="year">Minute</label><input type="text" name="minute" id="app-minute" value="' + time[1] + '"></div>';
			content += '<div><label for="year">Second</label><input type="text" name="second" id="app-second" value="' + time[2] + '"></div>';
			content += '<div><button type="button" name="set" id="app-rtc-set"> Set </button></div>';
			content += '</div>';
			$('#content').html(content);
		});
	}
	
	
	function setRTC() {
		$.ajax({
			url: '/api/rtc',
			method: 'POST',
			data:{
				year: $('#app-year').val(),
				month: $('#app-month').val(),
				day: $('#app-day').val(),
				dow: $('#app-dow').val(),
				hour: $('#app-hour').val(),
				minute: $('#app-minute').val(),
				second: $('#app-second').val()
			},
			dataType: 'json'
		}).done(function (data) {
			document.location.reload();
		});
	}
	
	
	function setConfig() {
		var data = {};
		$.each($('form.app-save-config input'), function (i, el) {
			var $el = $(el);
			data[$el.attr('name')] = $el.val();
		});
		$.ajax({
			url: '/api/config',
			method: 'POST',
			data: data,
			dataType: 'json'
		}).done(function (data) {
			document.location.reload();
		});
	}
	
	
	
	
	$("#app-sd").on('click', function(e) {
		e.preventDefault();
		resetSD();
		return false;
	});
	

	$("#app-rtc").on('click', function(e) {
		e.preventDefault();
		getRTC();
		return false;
	});
	
	
	
	$('body').on('click', "#app-rtc-set", function(e) {
		e.preventDefault();
		setRTC();
		return false;
	});
	
	
	$('body').on('click', "#app-config-set", function(e) {
		e.preventDefault();
		setConfig();
		return false;
	});
	
	
	var editableStatusFields = {"soilSensorMinLevel":true,"soilSensorMaxLevel":true,"timeReadMilisStandBy":true,"timeReadMilisWatering":true,"timeWarmingMilis":true,"mqttIp":true,"apiIp":true,"wifiIp":true,"wifiNet":true,"wifiGW":true,"wifiDNS1":true,"wifiDNS2":true,"wifi_mode":true};
	
	function _getField(n, v) {
		var content = '';
		if (v !== null && typeof v == 'object') {
			$.each(v, function (ns, vs) {
				content += _getField(ns, vs);
			});
		} else {
			content = '<div><label for="' + n + '">' + n + '</label>';
			if (typeof editableStatusFields[n] !== 'undefined' && editableStatusFields[n]) {
				content += '<input type="text" name="' + n + '" value="' + (v === null ? '' : v) + '">';
			} else {
				content += '<span> ' + v + '</span>';
			}
			content += '</div>';
		}
		return content;
	}
	
	
	
	function getStatus(force) {
		$.ajax({
			url: '/api/status',
			method: 'GET',
			dataType: 'json'
		}).done(function (data) {
			if (force || $('#content .status').length) {
				var content = '<h2>Status</h2>';
				content += '<div class="status">';
				$.each(data, function (n, v) {
					content += _getField(n, v);
				});
			$('#content').html(content + "</div>");
			}
		});
	}
	
	$('.app-status').on('click', function (e) { e.preventDefault(); getStatus(true); });
	
	setInterval(function() {
		if ($('#content .status').length) {
			getStatus(false);
		}
	}, 30000);
		
	function getConfig() {
		$.ajax({
			url: '/api/config',
			method: 'GET',
			dataType: 'json'
		}).done(function (data) {
				var content = '<h2>Config</h2><form class="app-save-config">';
				content += '<div class="config">';
				$.each(data, function (n, v) {
					content += _getField(n, v);
				});
			content += '<div><button type="button" name="set" id="app-config-set"> Save configuration </button></div></div></form>';
			$('#content').html(content);
		});
	}
	
	$('.app-config').on('click', function (e) { e.preventDefault(); getConfig(); });	
}());

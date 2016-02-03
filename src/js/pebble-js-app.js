/*jshint -W069*/

var tltoken = '';

function get_day_half() {
    return (new Date().getHours() > 12) ? 'PM' : 'AM';
}

function gen_time_string(hoursinfuture) {
    console.log('--------------------------------------------------------');
    console.log('generating time string for ' + hoursinfuture);
    var now = new Date();
    console.log('generating time string for ' + hoursinfuture + ' - date is: ' + now.toLocaleString());
    now.setHours((now.getHours() - (now.getHours() % 12)) + hoursinfuture);
    console.log('generating time string for ' + hoursinfuture + ' - date is: ' + now.toLocaleString());
    var time = now.toISOString().substring(0, 10);
    console.log('generating time string for ' + hoursinfuture + ': ' + time.toLocaleString());
    console.log('hours: ' + now.getHours());
    time += '-TP-';
    time += (now.getHours() >= 12) ? 'PM' : 'AM';
    console.log('generated time string for ' + hoursinfuture + ': ' + time.toLocaleString());
    console.log('--------------------------------------------------------');
    return time;
}

function send_tl_token(token) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            console.log(this.responseText);
        }
    };
    xhr.open('GET', 'https://pythonbackend-stepupforpebble.rhcloud.com/' +
                    'set_timeline_token?uid=' + Pebble.getAccountToken() +
                    '&tltoken=' + token);
    xhr.send();
    // It's possible that this fails.
    // Failing is ignored for a reason:
    //     If this fails, that means that
    //     the user doesn't exist.
    //     On the next launch, the app will
    //     retry sending the timeline token.
}

function send_pin(data) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            console.log('Pin push response: ' + this.responseText);
            console.log('Pin push status code: ' + this.status);
        }
    };
    xhr.open('PUT', 'https://timeline-api.getpebble.com/v1/user/pins/' + data.id);
    xhr.setRequestHeader('X-User-Token', tltoken);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(data));
    console.log(JSON.stringify(data));
}

function send_twice_daily_pin(offset) {
    var timestr = gen_time_string(offset);
    console.log(timestr);
    var hourofday = timestr.slice(-2) == "AM" ? 6 : 12;
    var xhr = new XMLHttpRequest();
    var timeExploded = timestr.slice(0, 10).split('-');
    var pinTime = new Date(timeExploded[0], timeExploded[1],
                           timeExploded[2], hourofday, 0, 0, 0);
    xhr.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                var json = JSON.parse(this.responseText);
                var challenge = json[0];
                var pin = {
                    'id': 'stepup-pin-' + timestr,
                    'time': pinTime.toISOString(),
                    'layout': {
                        'type': 'genericPin',
                        'title': 'StepUp Health Challenge!',
                        'backgroundColor': 'vividcerulean',
                        'subtitle': 'Beat: ' + challenge.username,
                        'body': challenge.username + ' reached ' + challenge.steps +
                                ' steps last ' +
                                (timestr.slice(-2) == 'AM' ? 'morning' : 'afternoon') +
                                '. Can you one-up them?',
                        'headings': ['Debug data:'],
                        'paragraphs': ['time: ' + pinTime.toISOString() +
                                       ' (' + timestr + '); pushed ' + offset +
                                       'h to future at: ' + new Date().toLocaleString() + ' (' + gen_time_string(0) + ')'],
                        'tinyIcon': 'system://images/STOCKS_EVENT'
                    }
                };
                console.log(JSON.stringify(challenge));
                console.log(JSON.stringify(pin));
                send_pin(pin);
            } else {
                console.log('vvvv Failure getting user info vvvv');
                console.log(this.responseText);
                console.log('^^^^ Failure getting user info ^^^^');
            }
        } else {
            console.error(this.responseText);
        }
    };
    xhr.open('GET', 'https://pythonbackend-stepupforpebble.rhcloud.com/' +
                    'get_active_friends?uid=' + Pebble.getAccountToken() +
                    '&dayhalf=' + timestr.slice(-2));
    xhr.send();
}

function send_hist_data(data) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            console.log(this.responseText);
        }
    };
    var time = gen_time_string(0);
    xhr.open('GET', 'https://pythonbackend-stepupforpebble.rhcloud.com/' +
                    'add_data_point?uid=' + Pebble.getAccountToken() +
                    '&steps=' + data + '&timeperiod=' + time);
    xhr.send();
}

function fetch_other_user_data() {
    var xhr = new XMLHttpRequest();
    var dayhalf = get_day_half();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            if (this.status == 200) {
                var data = {'MSG_KEY_DATA_SIZE': 0};
                var json = JSON.parse(this.responseText);
                if (json.length > 0) {
                    data['MSG_KEY_DATA_SIZE'] = Math.min(5, json.length);
                    for (var i = 0; i < Math.min(5, json.length); i++) {
                        data[100 + i] = json[i].username;
                        data[200 + i] = json[i].steps;
                    }
                }
                console.log(JSON.stringify(data));
                Pebble.sendAppMessage(data);
            }
        } else {
            console.error(this.responseText);
        }
    };
    xhr.open('GET', 'https://pythonbackend-stepupforpebble.rhcloud.com/' +
                    'get_active_friends?uid=' + Pebble.getAccountToken() +
                    '&dayhalf=' + dayhalf);
    xhr.send();
}

Pebble.addEventListener('appmessage', function(data) {
    console.log('AppMessage received');
    console.log(data.payload[25973]);
    send_hist_data(data.payload[25973]);
});

Pebble.addEventListener('showConfiguration', function() {
    var url = 'https://step-up-health.github.io/step-up-config-page/#' +
        Pebble.getAccountToken() + ':' + tltoken;
    console.log('Showing configuration page: ' + url);
    Pebble.openURL(url);
});

Pebble.addEventListener('ready', function() {
    send_twice_daily_pin(0);
    send_twice_daily_pin(12);
    send_twice_daily_pin(24);
    send_twice_daily_pin(36);
    send_twice_daily_pin(48);
    var uid = Pebble.getAccountToken();
    console.log('Hello');
    localStorage.tlTokenSent = localStorage.tlTokenSent || false;
    Pebble.getTimelineToken(function(token) {
        tltoken = token;
        send_tl_token(token);
    }, function(error) {
        console.error(error);
    });
    fetch_other_user_data();
});

Pebble.addEventListener('webviewclosed', function() {
    console.log('sending tl token because webviewclosed');
    Pebble.getTimelineToken(function(token) {
        console.log('sent tl token.');
        tltoken = token;
        send_tl_token(token);
    }, function(error) {
        console.error(error);
    });
});

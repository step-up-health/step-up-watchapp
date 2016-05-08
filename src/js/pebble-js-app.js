/*jshint -W069*/

var tltoken = '';
var base_url = 'https://pythonbackend-stepupforpebble.rhcloud.com/v1/';

function stringpad(str, char, amt) {
    str = '' + str;
    while (str.length < amt) {
        str = char + str;
    }
    return str;
}

function get_day_half() {
    return (new Date().getHours() > 12) ? 'PM' : 'AM';
}

function gen_time_string(hoursinfuture) {
    console.log('GTS ===> hif: ' + hoursinfuture);
    var now = new Date();
    now.setHours((now.getHours() - (now.getHours() % 12)) + hoursinfuture);
    var time = stringpad(now.getFullYear(), '0', 4) + '-' +
               stringpad((now.getMonth() + 1), '0', 2) + '-' +
               stringpad((now.getDate()), '0', 2);
    time += '-TP-';
    time += (now.getHours() >= 12) ? 'PM' : 'AM';
    console.log('GTS ===> ' + time);
    return time;
}

function send_tl_token(token) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            console.log(this.responseText);
        }
    };
    xhr.open('GET', base_url + 'set_timeline_token?uid=' +
                    Pebble.getAccountToken() + '&tltoken=' + token);
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

function send_twice_daily_pin(timestr) {
    console.log('Send twice daily pin - timestr:', timestr);
    var hourofday = timestr.slice(-2) == "AM" ? 6 : 12;
    var xhr = new XMLHttpRequest();
    var timeExploded = timestr.slice(0, 10).split('-');
    var pinTime = new Date(timeExploded[0], timeExploded[1] - 1,
                           timeExploded[2], hourofday, 0, 0, 0);
    xhr.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                var json = JSON.parse(this.responseText);
                if (json.length === 0) {
                    return;
                }
                json.sort(function(a, b) { return b.steps - a.steps; });
                console.log(JSON.stringify(json));
                var challenge = json[0];
                if (!('username' in challenge)) {
                    // Challenge is actually useless.
                    // The user should get some friends.
                    return;
                }
                var pin = {
                    'id': 'stepup-challenge-pin-' + timestr,
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
                        // 'headings': ['Debug data:'],
                        // 'paragraphs': ['time: ' + pinTime.toISOString() +
                        //                ' (' + timestr + '); pushed at: ' + new Date().toLocaleString() + ' (' + gen_time_string(0) + ')'],
                        'tinyIcon': 'system://images/STOCKS_EVENT'
                    }
                };
                console.log(JSON.stringify(challenge));
                console.log(JSON.stringify(pin));
                console.log('---- Sending pin ----');
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
    xhr.open('GET', base_url + 'get_active_friends?uid=' +
                    Pebble.getAccountToken() + '&dayhalf=' + timestr.slice(-2));
    xhr.send();
}

function send_applicable_pins() {
    send_twice_daily_pin(gen_time_string(12));
    send_twice_daily_pin(gen_time_string(24));
}

function send_hist_data(data) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            console.log(this.responseText);
        }
    };
    var time = gen_time_string(0);
    xhr.open('GET', base_url + 'add_data_point?uid=' +
                    Pebble.getAccountToken() + '&steps=' + data +
                    '&timeperiod=' + time);
    xhr.send();
}

function fetch_other_user_data() {
    var xhr = new XMLHttpRequest();
    var dayhalf = get_day_half();
    xhr.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            if (this.status == 200) {
                var data = {'MSG_KEY_DATA_SIZE': 0};
                console.log(this.responseText);
                var json = JSON.parse(this.responseText);
                if (json.length > 0) {
                    json.sort(function(a, b) { return b.steps - a.steps; });
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
    xhr.open('GET', base_url + 'get_active_friends?uid=' +
                    Pebble.getAccountToken() + '&dayhalf=' + dayhalf);
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
    send_applicable_pins();
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

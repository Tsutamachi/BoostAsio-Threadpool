// 该方法用于发送登录请求
function loginRequest(username, password, callback) {
    var request = new XMLHttpRequest()
    request.open("POST", "http://localhost:8080/user_login")

    request.setRequestHeader("Content-Type", "application/json")
    let data = {
        "email": username,
        "passwd": password
    }

    request.send(JSON.stringify(data))

    request.onreadystatechange = function () {
        if (request.readyState === XMLHttpRequest.DONE) {
            let response = JSON.parse(request.responseText)
            if (response.error === 0) {
                console.log("成功")
                // 这里的messagelabal指的是loginpage里面的label类型
                // messageLabel.text = "✅ 登录成功，欢迎 " + response.role + ": " + response.user
            } else {
                console.log("失败")
                // messageLabel.text = "❌ " + response.message
            }
            callback(response)
        }
    }
}

// 该方法用于发送登录请求
function varifiyRequest(email, callback) {
    var request = new XMLHttpRequest()
    request.open("POST", "http://localhost:8080/get_varifycode")
    request.setRequestHeader("Content-Type", "application/json")
    let data = {
        "email": email,
    }

    // 发送请求
    // 发送包裹
    // stringify将js对象转换为json字符串
    request.send(JSON.stringify(data))

    request.onreadystatechange = function () {
        if (request.readyState === XMLHttpRequest.DONE) {
            let response = JSON.parse(request.responseText)
            if (response.error === 0) {
                console.log("成功")
                // 这里的messagelabal指的是loginpage里面的label类型
                // messageLabel.text = "✅ 登录成功，欢迎 " + response.role + ": " + response.user
            } else {
                console.log("失败")
                // messageLabel.text = "❌ " + response.message
            }
            callback(response)
        }
    }
}

//请求发送邮箱验证码
function registerRequest(email,name,passwd,confirm,server,varifycode,callback) {
    var request = new XMLHttpRequest()
    request.open("POST", "http://localhost:8080/user_register")
    request.setRequestHeader("Content-Type", "application/json")
    let data = {
        "email": email,
        "name":name,
        "passwd":passwd,
        "confirm":confirm,
        "server":server,
        "varifycode":varifycode,
    }

    // 发送请求
    // 发送包裹
    // stringify将js对象转换为json字符串
    request.send(JSON.stringify(data))

    request.onreadystatechange = function () {
        if (request.readyState === XMLHttpRequest.DONE) {
            let response = JSON.parse(request.responseText)
            if (response.error === 0) {
                console.log("成功")
                // 这里的messagelabal指的是loginpage里面的label类型
                // messageLabel.text = "✅ 登录成功，欢迎 " + response.role + ": " + response.user
            } else {
                console.log("失败")
                // messageLabel.text = "❌ " + response.message
            }
            callback(response)
        }
    }
}

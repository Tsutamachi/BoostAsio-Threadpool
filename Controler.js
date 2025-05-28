// 该方法用于发送登录请求
function loginRequest(username, password, callback) {
    var request = new XMLHttpRequest()
    request.open("POST", "http://localhost:8080/user_login")

    request.setRequestHeader("Content-Type", "application/json")
    let data = {
        "email": username,
        "passwd": password
    }

    // 发送请求
    // 发送包裹
    // stringify将js对象转换为json字符串
    request.send(JSON.stringify(data))
    request.onreadystatechange = function () {
        if (request.readyState === XMLHttpRequest.DONE) {
            try{
                let response = JSON.parse(request.responseText)
                if (response.error === 0) {
                    console.log("成功")
                } else {
                    console.log("失败")
                }
                callback(response)
            }
            catch(error){
                console.log("服务器未开启,请先开始服务器完成登陆")
                firstlogin.loginErrorText.text="为检测到启动的服务器，请先启动服务器之后完成登陆"
                firstlogin.loginErrorText.visible=true
            }


        }
    }
}

// 该方法用于发送验证码请求
function varifiyRequest(email,server, callback) {

    try{
        var request = new XMLHttpRequest()
        const url = `http://${server}:8080/get_varifycode`;
        request.open("POST", url)
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
                try{
                    let response = JSON.parse(request.responseText)
                    if (response.error === 0) {
                        console.log("成功")
                    } else {
                        console.log("失败")
                    }
                    callback(response)
                }
                catch(error){
                    // console.log("指定服务器不存在或")
                    resgister.loginErrorText.text="连接指定服务器失败请检查，\n必须指定服务器后才能进行邮箱验证服务！"
                    resgister.loginErrorText.visible=true
                }
            }
        }
    }
    catch(e){
        resgister.loginErrorText.text="连接指定服务器失败请检查，\n必须指定服务器后才能进行邮箱验证服务！"
        resgister.loginErrorText.visible=true
    }

}

//请求发送邮箱验证码
function registerRequest(email,name,passwd,confirm,server,varifycode,callback) {
    var request = new XMLHttpRequest()
    const url = `http://${server}:8080/user_register`;
    request.open("POST", url)
    request.setRequestHeader("Content-Type", "application/json")
    let data = {
        "email": email,
        "user":name,
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

// 导入头文件
const nodemailer = require('nodemailer');
const config_module = require("./config")

/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    // 验证邮箱
    auth: {
        user: config_module.email_user, // 发送方邮箱地址
        pass: config_module.email_pass // 邮箱授权码或者密码
    }
});

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns 
 */

// resolve：是一个函数，当异步操作成功完成时调用它，并将操作的结果作为参数传递给它。
// reject：也是一个函数，当异步操作失败时调用它，并将错误信息作为参数传递给它。

function SendMail(mailOptions_){
    // 将异步回调变为同步
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })
   
}
// 导出这个模块
module.exports.SendMail = SendMail

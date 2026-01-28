// 相当于包含头文件
const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email');
const redis_module=require('./redis');
const config_module = require("./config");

async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try{
        let query_res = await redis_module.GetRedis(const_module.code_prefix+call.request.email);
        console.log("query_res is ", query_res)
        // 判断是否已经有了申请如果有了那么就直接赋值
        let uniqueId = query_res;
        if(query_res ==null){
            // 如果没有那么生成
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                // 截取长度
                uniqueId = uniqueId.substring(0, 4);
            }
            // 600s的过期时间，将整个key和value都设置在redis里
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix+call.request.email, uniqueId,180)
            if(!bres){
                // 如果有误就返回错误
                callback(null, { email:  call.request.email,
                    error:const_module.Errors.RedisErr
                });
                return;
            }
        }

        console.log("uniqueId is ", uniqueId)
        let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        //发送邮件
        let mailOptions = {
            from: config_module.email_user,
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };

        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Success
        });


    }catch(error){
        console.log("catch error is ", error)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        });
    }

}

function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    // 绑定监听的地址和端口
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        console.log('grpc server started')
    })
}

main()

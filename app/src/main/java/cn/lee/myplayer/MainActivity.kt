package cn.lee.myplayer

import android.Manifest
import android.app.NativeActivity
import android.content.pm.PackageManager
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.net.Uri
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import cn.lee.nativelib.NativeLib
import kotlinx.android.synthetic.main.activity_main.*
import androidx.core.content.ContextCompat
import androidx.annotation.NonNull
import androidx.core.app.ActivityCompat


public class MainActivity : AppCompatActivity() {
 //   var videoUrl = "http://n1cdn.miaopai.com/stream/Ba8ukPjIeAdBjTaOdWx9QdLhWngzw1A38xx2gg___32.mp4?ssig=4f116e7f2838eb0d0b7877f1f7c1f02c&time_stamp=1645708561081&unique_id=1645686960111380"
    var permissions = arrayOf(
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE
    )


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        checkPer()
        button.setOnClickListener {

            val holder = surfaceView.holder
            var videoUrl = "/sdcard/Movies/Test.mp4"
            val parse = Uri.parse(videoUrl)
            var native = NativeLib()

           var input = "/sdcard/Movies/aaaa.mp3"
           var  output = "/sdcard/Movies/output.pcm"
            Thread(Runnable {
                   native.playLocalVideo(videoUrl,holder.surface)
              //  native.sound(input,output)
                //   NativeJavaLib().testJni()

            }).start()
            Thread(Runnable {
             //   native.playLocalVideo(videoUrl,holder.surface)
            //    native.sound(videoUrl,output)
             //   NativeJavaLib().testJni()

            }).start()
        }

    }





    private fun checkPer() {
      var checked =   checkPermissionAllGranted(permissions)
        if (checked) {
            Log.d("err","所有权限已经授权！");
        }else{
            // 一次请求多个权限, 如果其他有权限是已经授予的将会自动忽略掉
            ActivityCompat.requestPermissions(this,
                permissions, 1024)
        }
    }


    /**
     * 检查是否拥有指定的所有权限
     */
    private fun checkPermissionAllGranted(permissions: Array<String>): Boolean {
        for (permission in permissions) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    permission
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                // 只要有一个权限没有被授予, 则直接返回 false
                Log.e("err", "权限" + permission + "没有授权")
                return false
            }
        }
        return true
    }


    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String?>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1024) {
            var isAllGranted = true
            // 判断是否所有的权限都已经授予了
            for (grant in grantResults) {
                if (grant != PackageManager.PERMISSION_GRANTED) {
                    isAllGranted = false
                    break
                }
            }
            if (isAllGranted) {
                // 所有的权限都授予了
                Log.e("err", "权限都授权了")
            } else {
                Log.e("err", "权限没有授权")
            }
        }
    }

}

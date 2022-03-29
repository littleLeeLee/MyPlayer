package cn.lee.nativelib

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import android.view.Surface
import android.view.SurfaceView

class NativeLib {

    /**
     * A native method that is implemented by the 'nativelib' native library,
     * which is packaged with this application.
     */

    external fun playLocalVideo(url:String,surface: Surface)

    external fun sound(input:String,output:String)

    fun testJni(s:String?,i:Int){

        Log.d("test","成功啦")


    }


    companion object {
        // Used to load the 'nativelib' library on application startup.
        init {
            System.loadLibrary("nativelib")
        }
    }

    private var audioTrack: AudioTrack? = null

    fun createAudio(sampleRateInHz : Int ,nb_channals :Int){
        Log.d("111","createAudio")
        val channaleConfig: Int
        channaleConfig = if (nb_channals === 1) {
            AudioFormat.CHANNEL_OUT_MONO
        } else if (nb_channals === 2) {
            AudioFormat.CHANNEL_OUT_STEREO
        } else {
            AudioFormat.CHANNEL_OUT_MONO
        }

        val bufferSize = AudioTrack.getMinBufferSize(
            sampleRateInHz,
            channaleConfig, AudioFormat.ENCODING_PCM_16BIT
        )
        audioTrack = AudioTrack(
            AudioManager.STREAM_MUSIC,
            sampleRateInHz, channaleConfig,
            AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM
        )

        audioTrack!!.play()
    }

    @Synchronized
    public fun playTrack(buffer : ByteArray,lenth : Int) {
        Log.d("111","playTrack")
        if (audioTrack != null && audioTrack!!.playState == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack!!.write(buffer, 0, lenth)
        }
    }


}
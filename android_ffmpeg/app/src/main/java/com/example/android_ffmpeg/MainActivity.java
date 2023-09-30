package com.example.android_ffmpeg;

import android.Manifest;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.TextView;

import com.example.android_ffmpeg.databinding.ActivityMainBinding;
import com.example.android_ffmpeg.widget.FFVideoView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private static final int REQUEST_PERMISSION_CODE = 123; // 可以自定义

    // Used to load the 'android_ffmpeg' library on application startup.
    static {
        System.loadLibrary("android_ffmpeg");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        mTextView= binding.sampleText;
        mVideoView=binding.videoView;

        // 检查读取外部存储的权限
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            // 如果没有权限，请求权限
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,
                            Manifest.permission.WRITE_EXTERNAL_STORAGE},

                    REQUEST_PERMISSION_CODE);
        }

    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        if (requestCode == REQUEST_PERMISSION_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // 用户已授予权限
            } else {
                // 用户拒绝了权限请求，您可以向用户解释为什么需要这些权限
            }
        }
    }
    public void onButtonClick(View view) {
        int id = view.getId();

        String videoPath = Environment.getExternalStorageDirectory() + "/DCIM/Camera/jojo.mp4";

        File f=new File(videoPath);
        if (f.exists()){
            boolean a=f.canRead();


            mVideoView.playVideo(videoPath);
        }

    }

    private void setInfoText(String content) {
        if (mTextView != null) {
            mTextView.setText(content);
        }
    }

    private TextView mTextView;
    private FFVideoView mVideoView;

}
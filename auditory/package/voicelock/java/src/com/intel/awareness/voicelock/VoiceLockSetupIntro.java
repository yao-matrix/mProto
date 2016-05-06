/*-------------------------------------------------------------------------
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * This source code and all documentation related to the source code
 * ("Material") contains trade secrets and proprietary and confidential
 * information of Intel and its suppliers and licensors. The Material is
 * deemed highly confidential, and is protected by worldwide copyright and
 * trade secret laws and treaty provisions. No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *-------------------------------------------------------------------------
 * Author: Sun, Taiyi (taiyi.sun@intel.com)
 * Create Time: 2013/04/18
 */

package com.intel.awareness.voicelock;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public final class VoiceLockSetupIntro extends PreferenceActivity {
    public Intent getIntent() {
        Intent localIntent = new Intent(super.getIntent());
        localIntent.putExtra(EXTRA_SHOW_FRAGMENT,
                VoiceLockSetupIntroFragment.class.getName());
        localIntent.putExtra(EXTRA_NO_HEADERS, true);
        return localIntent;
    }

    public static class VoiceLockSetupIntroFragment extends PreferenceFragment
            implements View.OnClickListener {
        private final String TAG = "VoiceLockSetupIntro";
        private Button mCancelButton;
        private Button mContinueButton;
        private TextView mHeaderText;
        private TextView mInfoText;
        private boolean mShowTutorial;
        private boolean mTutorialShowing;

        private void displayIntro() {
            this.mTutorialShowing = false;
            getActivity().setTitle(R.string.tutorial_app_name);
            this.mHeaderText.setText(R.string.intro_header);
            this.mInfoText.setText(R.string.intro_info_text);
            this.mCancelButton.setText(android.R.string.cancel);
            this.mContinueButton.setText(R.string.intro_continue_label);
        }

        private void displayTutorial() {
            this.mTutorialShowing = true;
            getActivity().setTitle(R.string.tutorial_app_name);
            this.mHeaderText.setText(R.string.tutorial_header);
            this.mInfoText.setText(R.string.tutorial_info_text);
            this.mCancelButton.setText(android.R.string.cancel);
            this.mContinueButton.setText(R.string.tutorial_continue_label);
        }

        public void onClick(View paramView) {
            if (paramView.getId() != R.id.footerLeftButton) {
                if (paramView.getId() == R.id.footerRightButton) {
                    if (!this.mTutorialShowing) {
                        Intent localIntent = new Intent(getActivity(),
                                VoiceLockSetup.class);
                        localIntent
                                .addFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT);
                        localIntent.putExtra("PendingIntent",
                                (PendingIntent) getActivity().getIntent()
                                        .getParcelableExtra("PendingIntent"));
                        startActivity(localIntent);
                        getActivity().finish();
                    } else {
                        this.mShowTutorial = false;
                        displayIntro();
                    }
                } else
                    throw new AssertionError();
            } else
                getActivity().finish();
        }

        public void onCreate(Bundle paramBundle) {
            super.onCreate(paramBundle);
            if (paramBundle != null)
                this.mShowTutorial = paramBundle.getBoolean("showTutorial");
            else
                this.mShowTutorial = getActivity().getIntent().getBooleanExtra(
                        "showTutorial", false);
        }

        public View onCreateView(LayoutInflater paramLayoutInflater,
                ViewGroup paramViewGroup, Bundle paramBundle) {
            View localView = null;
            try {
                localView = paramLayoutInflater.inflate(
                    R.layout.setup_voice_lock_intro, paramViewGroup, false);

                //For Klockwork Scan
                if (localView == null)
                    return localView;

                this.mHeaderText = ((TextView) localView
                        .findViewById(R.id.headerText));
                this.mInfoText = ((TextView) localView.findViewById(R.id.infoText));
                this.mCancelButton = ((Button) localView
                        .findViewById(R.id.footerLeftButton));
                this.mContinueButton = ((Button) localView
                        .findViewById(R.id.footerRightButton));

                //For Klockwork Scan
                if (mHeaderText == null || mInfoText == null || mCancelButton == null || mContinueButton == null)
                    return localView;

                this.mCancelButton.setOnClickListener(this);
                this.mContinueButton.setOnClickListener(this);
                if (!this.mShowTutorial)
                    displayIntro();
                else
                    displayTutorial();
            } catch (Exception exp) {
                exp.printStackTrace();
            }
            return localView;
        }

        public void onDestroy() {
            super.onDestroy();
        }

        public void onPause() {
            super.onPause();
        }

        public void onResume() {
            super.onResume();
        }

        public void onSaveInstanceState(Bundle paramBundle) {
            paramBundle.putBoolean("showTutorial", this.mShowTutorial);
            super.onSaveInstanceState(paramBundle);
        }
    }
}

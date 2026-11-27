// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;

public abstract class AbstractSchemaTest<T extends MessageLite> {
  private Schema<T> schema;

  @Before
  public void setup() {
    schema = schema();
  }

  protected abstract Schema<T> schema();

  protected abstract ExperimentalMessageFactory<? extends T> messageFactory();

  @SuppressWarnings("unused")
  protected List<ByteBuffer> serializedBytesWithInvalidUtf8() throws IOException {
    return Collections.emptyList();
  }

  @Test
  public void randomMessageShouldRoundtrip() throws IOException {
    roundtrip("", messageFactory().newMessage());
  }

  @Test
  public void invalidUtf8StringParsing() throws IOException {
    for (ByteBuffer invalidUtf8Bytes : serializedBytesWithInvalidUtf8()) {
      CodedInputStreamReader reader =
          CodedInputStreamReader.forCodedInput(CodedInputStream.newInstance(invalidUtf8Bytes));

      T newMsg = schema.newInstance();
      try {
        schema.mergeFrom(newMsg, reader, ExtensionRegistryLite.getEmptyRegistry());
        assertWithMessage("should throw invalid").fail();
      } catch (InvalidProtocolBufferException expected) {
      }
    }
  }


  protected static final <M extends MessageLite> void roundtrip(
      String failureMessage, M msg, Schema<M> schema) throws IOException {
    byte[] serializedBytes = msg.toByteArray();
    assertWithMessage(failureMessage)
        .that(serializedBytes.length)
        .isEqualTo(msg.getSerializedSize());

    // Now read it back in and verify it matches the original.
    M newMsg = schema.newInstance();
    CodedInputStreamReader reader =
        CodedInputStreamReader.forCodedInput(CodedInputStream.newInstance(serializedBytes));
    schema.mergeFrom(newMsg, reader, ExtensionRegistryLite.getEmptyRegistry());
    schema.makeImmutable(newMsg);

    assertWithMessage(failureMessage).that(newMsg).isEqualTo(msg);
  }

  protected final void roundtrip(String failureMessage, T msg) throws IOException {
    roundtrip(failureMessage, msg, schema);
  }

  protected final ExperimentalTestDataProvider data() {
    return messageFactory().dataProvider();
  }

  protected List<T> newMessagesMissingRequiredFields() {
    return Collections.emptyList();
  }

  @SuppressWarnings("unchecked")
  @Test
  public void testRequiredFields() throws Exception {
    for (T msg : newMessagesMissingRequiredFields()) {
      if (schema.isInitialized(msg)) {
        assertThat(msg.toString()).isEmpty();
        msg = (T) msg.toBuilder().build();
      }
      assertThat(schema.isInitialized(msg)).isFalse();
    }
  }
}
